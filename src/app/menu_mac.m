/* menu_mac.m — the native macOS menu bar.
 *
 * WHY THIS EXISTS
 *   JellyDazzle is an SDL app, and SDL 2.30 already builds a minimal menu bar
 *   (App / Window / View) inside SDL_Init -> Cocoa_RegisterApp(). What it does
 *   NOT build is a Help menu, and macOS has no F1: the system shortcut for help
 *   is Command-Question Mark, reached from a Help menu that every Mac app is
 *   expected to have. A Mac app with no Help menu is a HIG gap a reviewer can
 *   see at a glance.
 *
 * WHY WE APPEND RATHER THAN BUILD OUR OWN
 *   SDL only calls CreateApplicationMenus() when [NSApp mainMenu] is nil, so
 *   installing a bar BEFORE SDL_Init would work — but it forces NSApp into
 *   existence first, and SDL then skips its whole registration block:
 *   no setActivationPolicy:Regular, no finishLaunching, no SDL event handling
 *   mode. That is a real regression for a Dock-visible app. So: let SDL build
 *   its bar, then adjust it.
 *
 * MEMORY
 *   The Makefile passes no -fobjc-arc (same as systap.m), so this is manual
 *   retain/release. NSMenuItem does NOT retain its target, which is why the
 *   target object is a static that lives for the life of the process.
 */
#import <Cocoa/Cocoa.h>

/* Raised here, consumed by the engine — the same contract as jd_req_palette. */
extern void jd_about_toggle(void);
extern int  jd_about_is_on(void);

@interface JDMenuTarget : NSObject
@end

@implementation JDMenuTarget
/* Runs on the main thread, synchronously inside SDL_PollEvent -> [NSApp
 * sendEvent:], i.e. between frames. Flipping an int is all we do; anything
 * heavier would belong in an SDL_PushEvent so it lands in the normal loop. */
- (void)jdAbout:(id)sender { (void)sender; jd_about_toggle(); }
- (void)jdHelp:(id)sender  { (void)sender; jd_about_toggle(); }

/* An explicit target plus an implemented selector is what makes AppKit
 * auto-enable the item; a nil target walks the responder chain, finds nothing
 * that implements these, and greys the item out. The checkmark mirrors whether
 * the card is currently up. */
- (BOOL)validateMenuItem:(NSMenuItem *)item
{
    [item setState:(jd_about_is_on() ? NSControlStateValueOn : NSControlStateValueOff)];
    return YES;
}
@end

static JDMenuTarget *jd_menu_target;   /* never released: app-lifetime */

void jd_menu_install(void)
{ @autoreleasepool {
    NSMenu *mainMenu = [NSApp mainMenu];
    if (!mainMenu || [mainMenu numberOfItems] == 0) return;   /* SDL_Init failed */

    jd_menu_target = [[JDMenuTarget alloc] init];

    /* ---- application menu (item 0; AppKit always renders the first item as
     * the app menu and substitutes the bundle name for its title) ---------- */
    NSMenu *appMenu = [[mainMenu itemAtIndex:0] submenu];

    /* SDL wires About to orderFrontStandardAboutPanel: with a nil target.
     * Find it by SELECTOR rather than assuming an index. */
    NSInteger ai = [appMenu indexOfItemWithTarget:nil
                                        andAction:@selector(orderFrontStandardAboutPanel:)];
    if (ai >= 0) {
        NSMenuItem *about = [appMenu itemAtIndex:ai];
        [about setTitle:@"About JellyDazzle"];
        [about setTarget:jd_menu_target];
        [about setAction:@selector(jdAbout:)];
        /* ⚠ Deliberately NO key equivalent — but no longer for the original
         * reason. AppKit fires a menu key equivalent AND still forwards the
         * key to SDL, and listen.c used to poll bare A, so Cmd-A toggled the
         * card twice and looked like a no-op. listen.c now ignores its entire
         * key poll while Command is down, so that double-fire is gone. The
         * item stays bare anyway: Cmd-A is Select All everywhere else on the
         * Mac and an About card is not worth stealing it for. Cmd-? on the
         * Help item is the sanctioned shortcut and passes the same gate. */
    }

    /* A non-bundled run (make run) titles these from the lowercase process
     * name; a bundled run uses CFBundleName. Normalise so both read the same. */
    NSInteger qi = [appMenu indexOfItemWithTarget:nil andAction:@selector(terminate:)];
    if (qi >= 0) [[appMenu itemAtIndex:qi] setTitle:@"Quit JellyDazzle"];
    /* Leave terminate: alone — SDLApplication overrides it to post SDL_QUIT,
     * which main.c already handles, so shutdown still runs jd_audio_close(). */

    /* ---- Help menu (rightmost) ------------------------------------------ */
    NSMenu *helpMenu = [[NSMenu alloc] initWithTitle:@"Help"];
    NSMenuItem *helpEntry = [helpMenu addItemWithTitle:@"JellyDazzle Help"
                                                action:@selector(jdHelp:)
                                         keyEquivalent:@"?"];
    /* Cmd-? : the character is "?" and the mask is Command ALONE. Using "/"
     * with Shift|Command renders as ⇧⌘/ instead; AppKit matches against the
     * shifted character, which is also why Apple calls this shortcut
     * "Command-Question Mark" rather than naming the slash key. */
    [helpEntry setKeyEquivalentModifierMask:NSEventModifierFlagCommand];
    [helpEntry setTarget:jd_menu_target];

    NSMenuItem *helpTop = [[NSMenuItem alloc] initWithTitle:@"Help"
                                                     action:NULL
                                              keyEquivalent:@""];
    [helpTop setSubmenu:helpMenu];
    [mainMenu addItem:helpTop];       /* appended => rightmost, after App/Window/View */
    /* TWO DELIBERATE OMISSIONS, AND THEY ARE THE SAME BUG: we never call
     * setHelpMenu:, and we never release helpTop or helpMenu.
     *
     * The releases look correct by the ownership rules — mainMenu retains
     * helpTop, helpTop retains helpMenu — but with setHelpMenu: also in play
     * the app shut ITSELF down about a minute after launch: WINDOWEVENT_CLOSE,
     * then SDL_QUIT, clean exit 0, no crash report, reproducible every run.
     * Bisected against a no-menu build side by side: dropping EITHER the
     * setHelpMenu: call or the releases fixes it, keeping both does not. So
     * this is an over-release — setHelpMenu: leaves AppKit holding a reference
     * that our release then drops out from under it. Either half alone is a
     * fix; we keep both out because neither costs us anything:
     *
     *   - The releases buy nothing. A menu bar lives as long as the app does,
     *     so there is no memory to reclaim. Same reasoning as jd_menu_target.
     *   - setHelpMenu: buys little. AppKit recognises a menu titled "Help" by
     *     itself and adds the help search field to it.
     *
     * ⚠ What setHelpMenu: DID buy is position, and nothing replaces that.
     * Help sits rightmost only because addItem: appends and no code runs after
     * us — SDL builds App/Window/View inside SDL_Init, before jd_menu_install.
     * A menu added later lands to the RIGHT of Help and breaks HIG order.
     * Insert at an explicit index rather than appending if you add one.
     *
     * ⚠ AppKit's auto-recognition matches the LOCALISED title, so hardcoding
     * @"Help" makes that behaviour English-only. This is the line to revisit
     * if JellyDazzle is ever localised. */
}}

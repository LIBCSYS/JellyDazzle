<?php
// Local-only weight sink for the JellyDazzle command panel.
if ($_SERVER['REQUEST_METHOD'] !== 'POST') { http_response_code(405); exit('POST only'); }
$raw = file_get_contents('php://input');
$j = json_decode($raw, true);
if (!is_array($j)) { http_response_code(400); exit('bad json'); }
file_put_contents(__DIR__.'/weights.json', json_encode($j, JSON_PRETTY_PRINT));
echo json_encode(['ok'=>true, 'count'=>count($j), 'saved'=>date('c')]);

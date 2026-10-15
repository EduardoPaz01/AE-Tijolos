<?php
// cors.php — incluir na PRIMEIRA linha de cada endpoint
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type');
header('Access-Control-Allow-Private-Network: true');
header('Access-Control-Max-Age: 600');

// Responde o preflight e para aqui, sem executar o resto do script
if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}
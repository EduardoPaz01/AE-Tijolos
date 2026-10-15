<?php
require __DIR__ .'/cors.php';
ini_set('display_errors', 1);
ini_set('display_startup_errors', 1);
error_reporting(E_ALL);

require_once 'conexao.php';
$con->set_charset("utf8");

header('Content-Type: application/json; charset=utf-8');

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    echo json_encode(["success" => false, "error" => "Método não permitido"], JSON_UNESCAPED_UNICODE);
    exit;
}

$data = json_decode(file_get_contents('php://input'), true);

if (!is_array($data) || !isset($data['idReceita']) || $data['idReceita'] === '') {
    http_response_code(400);
    echo json_encode(["success" => false, "error" => "Campo obrigatório ausente: idReceita"], JSON_UNESCAPED_UNICODE);
    exit;
}

$idReceita   = (int) $data['idReceita'];                                   // NOT NULL
$tempo       = isset($data['tempo']) ? (float) $data['tempo'] : null;      // DECIMAL(6,4)
$temperatura = isset($data['temperatura']) ? (float) $data['temperatura'] : null; // DECIMAL(6,2)
$nOrdem      = isset($data['nOrdem']) ? (int) $data['nOrdem'] : null;      // INT

$sql = "INSERT INTO Ponto (idReceita, tempo, temperatura, nOrdem) VALUES (?, ?, ?, ?)";
$stmt = $con->prepare($sql);

if (!$stmt) {
    http_response_code(500);
    echo json_encode(["success" => false, "error" => "Erro ao preparar: " . $con->error], JSON_UNESCAPED_UNICODE);
    exit;
}

$stmt->bind_param("iddi", $idReceita, $tempo, $temperatura, $nOrdem);

if ($stmt->execute()) {
    echo json_encode([
        "success" => true,
        "idPonto" => $stmt->insert_id,
        "message" => "Ponto inserido com sucesso"
    ], JSON_UNESCAPED_UNICODE);
} else {
    http_response_code(500);
    $msg = ($con->errno == 1452) ? "idReceita não existe" : $stmt->error;
    echo json_encode(["success" => false, "error" => $msg], JSON_UNESCAPED_UNICODE);
}

$stmt->close();
$con->close();
?>

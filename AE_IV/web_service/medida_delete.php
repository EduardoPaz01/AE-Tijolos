<?php
require __DIR__ .'/cors.php';
ini_set('display_errors', 1);
ini_set('display_startup_errors', 1);
error_reporting(E_ALL);

require_once 'conexao.php';
$con->set_charset("utf8");

header('Content-Type: application/json; charset=utf-8');

if (!in_array($_SERVER['REQUEST_METHOD'], ['POST', 'DELETE'])) {
    http_response_code(405);
    echo json_encode(["success" => false, "error" => "Método não permitido"], JSON_UNESCAPED_UNICODE);
    exit;
}

$data = json_decode(file_get_contents('php://input'), true);

if (!is_array($data) || !isset($data['idMedida']) || !is_numeric($data['idMedida'])) {
    http_response_code(400);
    echo json_encode(["success" => false, "error" => "Informe um idMedida válido"], JSON_UNESCAPED_UNICODE);
    exit;
}

$idMedida = (int) $data['idMedida'];

// Medida não tem tabelas filhas: exclusão direta
$stmt = $con->prepare("DELETE FROM Medida WHERE idMedida = ?");
$stmt->bind_param("i", $idMedida);

if ($stmt->execute()) {
    if ($stmt->affected_rows === 0) {
        http_response_code(404);
        echo json_encode(["success" => false, "error" => "Medida não encontrada"], JSON_UNESCAPED_UNICODE);
    } else {
        echo json_encode([
            "success"  => true,
            "idMedida" => $idMedida,
            "message"  => "Medida excluída com sucesso"
        ], JSON_UNESCAPED_UNICODE);
    }
} else {
    http_response_code(500);
    echo json_encode(["success" => false, "error" => $stmt->error], JSON_UNESCAPED_UNICODE);
}

$stmt->close();
$con->close();
?>

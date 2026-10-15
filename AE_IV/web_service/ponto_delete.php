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

if (!is_array($data) || !isset($data['idPonto']) || !is_numeric($data['idPonto'])) {
    http_response_code(400);
    echo json_encode(["success" => false, "error" => "Informe um idPonto válido"], JSON_UNESCAPED_UNICODE);
    exit;
}

$idPonto = (int) $data['idPonto'];

// Ponto não tem tabelas filhas: exclusão direta
$stmt = $con->prepare("DELETE FROM Ponto WHERE idPonto = ?");
$stmt->bind_param("i", $idPonto);

if ($stmt->execute()) {
    if ($stmt->affected_rows === 0) {
        http_response_code(404);
        echo json_encode(["success" => false, "error" => "Ponto não encontrado"], JSON_UNESCAPED_UNICODE);
    } else {
        echo json_encode([
            "success" => true,
            "idPonto" => $idPonto,
            "message" => "Ponto excluído com sucesso"
        ], JSON_UNESCAPED_UNICODE);
    }
} else {
    http_response_code(500);
    echo json_encode(["success" => false, "error" => $stmt->error], JSON_UNESCAPED_UNICODE);
}

$stmt->close();
$con->close();
?>

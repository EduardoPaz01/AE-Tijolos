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

if (!is_array($data) || !isset($data['idUsuario']) || $data['idUsuario'] === '') {
    http_response_code(400);
    echo json_encode(["success" => false, "error" => "Campo obrigatório ausente: idUsuario"], JSON_UNESCAPED_UNICODE);
    exit;
}

$titulo    = isset($data['titulo']) ? $data['titulo'] : null; // VARCHAR(100) NULL
$idUsuario = (int) $data['idUsuario'];                        // INT UNSIGNED NOT NULL

$sql = "INSERT INTO Receita (titulo, idUsuario) VALUES (?, ?)";
$stmt = $con->prepare($sql);

if (!$stmt) {
    http_response_code(500);
    echo json_encode(["success" => false, "error" => "Erro ao preparar: " . $con->error], JSON_UNESCAPED_UNICODE);
    exit;
}

$stmt->bind_param("si", $titulo, $idUsuario);

if ($stmt->execute()) {
    echo json_encode([
        "success"   => true,
        "idReceita" => $stmt->insert_id,
        "message"   => "Receita inserida com sucesso"
    ], JSON_UNESCAPED_UNICODE);
} else {
    http_response_code(500);
    $msg = ($con->errno == 1452) ? "idUsuario não existe" : $stmt->error;
    echo json_encode(["success" => false, "error" => $msg], JSON_UNESCAPED_UNICODE);
}

$stmt->close();
$con->close();
?>

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

if (!is_array($data) || empty($data['nome']) || empty($data['senha'])) {
    http_response_code(400);
    echo json_encode(["success" => false, "error" => "Informe nome e senha"], JSON_UNESCAPED_UNICODE);
    exit;
}

$nome = $data['nome'];
// A coluna é VARCHAR(45); um hash bcrypt tem 60 caracteres e não cabe.
// Se possível, aumente a coluna 'senha' para VARCHAR(255) e use password_hash().
$senha = $data['senha'];

// Verifica duplicidade de nome de usuário
$check = $con->prepare("SELECT idUsuario FROM Usuario WHERE nome = ?");
$check->bind_param("s", $nome);
$check->execute();
$check->store_result();
if ($check->num_rows > 0) {
    $check->close();
    http_response_code(409);
    echo json_encode(["success" => false, "error" => "Já existe um usuário com esse nome"], JSON_UNESCAPED_UNICODE);
    exit;
}
$check->close();

$sql = "INSERT INTO Usuario (nome, senha) VALUES (?, ?)";
$stmt = $con->prepare($sql);

if (!$stmt) {
    http_response_code(500);
    echo json_encode(["success" => false, "error" => "Erro ao preparar: " . $con->error], JSON_UNESCAPED_UNICODE);
    exit;
}

$stmt->bind_param("ss", $nome, $senha);

if ($stmt->execute()) {
    echo json_encode([
        "success"   => true,
        "idUsuario" => $stmt->insert_id,
        "message"   => "Usuário inserido com sucesso"
    ], JSON_UNESCAPED_UNICODE);
} else {
    http_response_code(500);
    echo json_encode(["success" => false, "error" => $stmt->error], JSON_UNESCAPED_UNICODE);
}

$stmt->close();
$con->close();
?>

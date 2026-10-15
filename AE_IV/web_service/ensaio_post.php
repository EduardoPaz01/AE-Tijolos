<?php
require __DIR__ .'/cors.php';
// Configurações de Erro
ini_set('display_errors', 1);
ini_set('display_startup_errors', 1);
error_reporting(E_ALL);

// Inclui o script de conexão existente
require_once 'conexao.php';
$con->set_charset("utf8");

header('Content-Type: application/json; charset=utf-8');

// Aceita apenas POST
if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    echo json_encode(["success" => false, "error" => "Método não permitido"], JSON_UNESCAPED_UNICODE);
    exit;
}

// Decodifica a entrada JSON
$data = json_decode(file_get_contents('php://input'), true);

if (!is_array($data)) {
    http_response_code(400);
    echo json_encode(["success" => false, "error" => "JSON inválido"], JSON_UNESCAPED_UNICODE);
    exit;
}

// Campos obrigatórios (FKs NOT NULL na tabela Ensaio)
foreach (['idReceita', 'idUsuario'] as $campo) {
    if (!isset($data[$campo]) || $data[$campo] === '') {
        http_response_code(400);
        echo json_encode(["success" => false, "error" => "Campo obrigatório ausente: $campo"], JSON_UNESCAPED_UNICODE);
        exit;
    }
}

// Campos da tabela
$nome      = isset($data['nome']) ? $data['nome'] : null;              // VARCHAR(100) NULL
$dataEnsaio= isset($data['data']) ? $data['data'] : date('Y-m-d H:i:s'); // VARCHAR(45) NULL
$idReceita = (int) $data['idReceita'];                                  // INT UNSIGNED NOT NULL
$idUsuario = (int) $data['idUsuario'];                                  // INT UNSIGNED NOT NULL

$sql = "INSERT INTO Ensaio (nome, data, idReceita, idUsuario) VALUES (?, ?, ?, ?)";

$stmt = $con->prepare($sql);

if (!$stmt) {
    http_response_code(500);
    echo json_encode(["success" => false, "error" => "Erro ao preparar: " . $con->error], JSON_UNESCAPED_UNICODE);
    exit;
}

// s = string, i = inteiro
$stmt->bind_param("ssii", $nome, $dataEnsaio, $idReceita, $idUsuario);

if ($stmt->execute()) {
    echo json_encode([
        "success"  => true,
        "idEnsaio" => $stmt->insert_id,
        "message"  => "Ensaio inserido com sucesso"
    ], JSON_UNESCAPED_UNICODE);
} else {
    http_response_code(500);
    // errno 1452 = falha de chave estrangeira
    $msg = ($con->errno == 1452)
        ? "idReceita ou idUsuario não existe"
        : $stmt->error;
    echo json_encode(["success" => false, "error" => $msg], JSON_UNESCAPED_UNICODE);
}

$stmt->close();
$con->close();

?>

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

if (!is_array($data) || !isset($data['idEnsaio']) || $data['idEnsaio'] === '') {
    http_response_code(400);
    echo json_encode(["success" => false, "error" => "Campo obrigatório ausente: idEnsaio"], JSON_UNESCAPED_UNICODE);
    exit;
}

$idEnsaio              = (int) $data['idEnsaio'];                                                  // NOT NULL
$tempo                 = isset($data['tempo']) ? (float) $data['tempo'] : null;                    // DECIMAL(6,4)
$temperaturaReal       = isset($data['temperaturaReal']) ? (float) $data['temperaturaReal'] : null; // DECIMAL(6,2)
$temperaturaReferencia = isset($data['temperaturaReferencia']) ? (float) $data['temperaturaReferencia'] : null; // DECIMAL(6,2)

$sql = "INSERT INTO Medida (idEnsaio, tempo, temperaturaReal, temperaturaReferencia) VALUES (?, ?, ?, ?)";
$stmt = $con->prepare($sql);

if (!$stmt) {
    http_response_code(500);
    echo json_encode(["success" => false, "error" => "Erro ao preparar: " . $con->error], JSON_UNESCAPED_UNICODE);
    exit;
}

$stmt->bind_param("iddd", $idEnsaio, $tempo, $temperaturaReal, $temperaturaReferencia);

if ($stmt->execute()) {
    echo json_encode([
        "success"  => true,
        "idMedida" => $stmt->insert_id,
        "message"  => "Medida inserida com sucesso"
    ], JSON_UNESCAPED_UNICODE);
} else {
    http_response_code(500);
    $msg = ($con->errno == 1452) ? "idEnsaio não existe" : $stmt->error;
    echo json_encode(["success" => false, "error" => $msg], JSON_UNESCAPED_UNICODE);
}

$stmt->close();
$con->close();
?>

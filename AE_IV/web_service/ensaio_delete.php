<?php
require __DIR__ .'/cors.php';
// Configurações de Erro
ini_set('display_errors', 1);
ini_set('display_startup_errors', 1);
error_reporting(E_ALL);

require_once 'conexao.php';
$con->set_charset("utf8");

header('Content-Type: application/json; charset=utf-8');

// Aceita POST ou DELETE
if (!in_array($_SERVER['REQUEST_METHOD'], ['POST', 'DELETE'])) {
    http_response_code(405);
    echo json_encode(["success" => false, "error" => "Método não permitido"], JSON_UNESCAPED_UNICODE);
    exit;
}

$data = json_decode(file_get_contents('php://input'), true);

if (!is_array($data) || !isset($data['idEnsaio']) || !is_numeric($data['idEnsaio'])) {
    http_response_code(400);
    echo json_encode(["success" => false, "error" => "Informe um idEnsaio válido"], JSON_UNESCAPED_UNICODE);
    exit;
}

$idEnsaio = (int) $data['idEnsaio'];

// Transação: apaga as medidas e o ensaio juntos, ou nada
$con->begin_transaction();

try {
    // 1) Verifica se o ensaio existe
    $stmt = $con->prepare("SELECT idEnsaio FROM Ensaio WHERE idEnsaio = ?");
    $stmt->bind_param("i", $idEnsaio);
    $stmt->execute();
    $stmt->store_result();

    if ($stmt->num_rows === 0) {
        $stmt->close();
        $con->rollback();
        http_response_code(404);
        echo json_encode(["success" => false, "error" => "Ensaio não encontrado"], JSON_UNESCAPED_UNICODE);
        exit;
    }
    $stmt->close();

    // 2) Apaga as medidas filhas (a FK é NO ACTION, então elas não somem sozinhas)
    $stmt = $con->prepare("DELETE FROM Medida WHERE idEnsaio = ?");
    $stmt->bind_param("i", $idEnsaio);
    if (!$stmt->execute()) {
        throw new Exception($stmt->error);
    }
    $medidasRemovidas = $stmt->affected_rows;
    $stmt->close();

    // 3) Apaga o ensaio
    $stmt = $con->prepare("DELETE FROM Ensaio WHERE idEnsaio = ?");
    $stmt->bind_param("i", $idEnsaio);
    if (!$stmt->execute()) {
        throw new Exception($stmt->error);
    }
    $stmt->close();

    $con->commit();

    echo json_encode([
        "success"          => true,
        "idEnsaio"         => $idEnsaio,
        "medidasRemovidas" => $medidasRemovidas,
        "message"          => "Ensaio excluído com sucesso"
    ], JSON_UNESCAPED_UNICODE);

} catch (Exception $e) {
    $con->rollback();
    http_response_code(500);
    echo json_encode([
        "success" => false,
        "error"   => $e->getMessage()
    ], JSON_UNESCAPED_UNICODE);
}

$con->close();

?>

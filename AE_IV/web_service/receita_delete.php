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

if (!is_array($data) || !isset($data['idReceita']) || !is_numeric($data['idReceita'])) {
    http_response_code(400);
    echo json_encode(["success" => false, "error" => "Informe um idReceita válido"], JSON_UNESCAPED_UNICODE);
    exit;
}

$idReceita = (int) $data['idReceita'];

// Apagar uma Receita derruba os Pontos dela e os Ensaios (com suas Medidas)
$con->begin_transaction();

try {
    $check = $con->prepare("SELECT idReceita FROM Receita WHERE idReceita = ?");
    $check->bind_param("i", $idReceita);
    $check->execute();
    $check->store_result();
    if ($check->num_rows === 0) {
        $check->close();
        $con->rollback();
        http_response_code(404);
        echo json_encode(["success" => false, "error" => "Receita não encontrada"], JSON_UNESCAPED_UNICODE);
        exit;
    }
    $check->close();

    $idsEnsaio = [];
    $stmt = $con->prepare("SELECT idEnsaio FROM Ensaio WHERE idReceita = ?");
    $stmt->bind_param("i", $idReceita);
    $stmt->execute();
    $res = $stmt->get_result();
    while ($r = $res->fetch_assoc()) { $idsEnsaio[] = (int) $r['idEnsaio']; }
    $stmt->close();

    if (!empty($idsEnsaio)) {
        $placeholders = implode(',', array_fill(0, count($idsEnsaio), '?'));
        $tipos = str_repeat('i', count($idsEnsaio));

        $stmt = $con->prepare("DELETE FROM Medida WHERE idEnsaio IN ($placeholders)");
        $stmt->bind_param($tipos, ...$idsEnsaio);
        $stmt->execute();
        $stmt->close();
    }

    $stmt = $con->prepare("DELETE FROM Ensaio WHERE idReceita = ?");
    $stmt->bind_param("i", $idReceita);
    $stmt->execute();
    $stmt->close();

    $stmt = $con->prepare("DELETE FROM Ponto WHERE idReceita = ?");
    $stmt->bind_param("i", $idReceita);
    $stmt->execute();
    $pontosRemovidos = $stmt->affected_rows;
    $stmt->close();

    $stmt = $con->prepare("DELETE FROM Receita WHERE idReceita = ?");
    $stmt->bind_param("i", $idReceita);
    if (!$stmt->execute()) {
        throw new Exception($stmt->error);
    }
    $stmt->close();

    $con->commit();
    echo json_encode([
        "success"          => true,
        "idReceita"        => $idReceita,
        "pontosRemovidos"  => $pontosRemovidos,
        "message"          => "Receita excluída com sucesso"
    ], JSON_UNESCAPED_UNICODE);

} catch (Exception $e) {
    $con->rollback();
    http_response_code(500);
    echo json_encode(["success" => false, "error" => $e->getMessage()], JSON_UNESCAPED_UNICODE);
}

$con->close();
?>

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

if (!is_array($data) || !isset($data['idUsuario']) || !is_numeric($data['idUsuario'])) {
    http_response_code(400);
    echo json_encode(["success" => false, "error" => "Informe um idUsuario válido"], JSON_UNESCAPED_UNICODE);
    exit;
}

$idUsuario = (int) $data['idUsuario'];

// Apagar um Usuario derruba toda a árvore: Receita -> Ponto, e Ensaio -> Medida.
// Sem isso, o DELETE final falha com erro de chave estrangeira (1451).
$con->begin_transaction();

try {
    $check = $con->prepare("SELECT idUsuario FROM Usuario WHERE idUsuario = ?");
    $check->bind_param("i", $idUsuario);
    $check->execute();
    $check->store_result();
    if ($check->num_rows === 0) {
        $check->close();
        $con->rollback();
        http_response_code(404);
        echo json_encode(["success" => false, "error" => "Usuário não encontrado"], JSON_UNESCAPED_UNICODE);
        exit;
    }
    $check->close();

    // IDs de receita deste usuário
    $idsReceita = [];
    $stmt = $con->prepare("SELECT idReceita FROM Receita WHERE idUsuario = ?");
    $stmt->bind_param("i", $idUsuario);
    $stmt->execute();
    $res = $stmt->get_result();
    while ($r = $res->fetch_assoc()) { $idsReceita[] = (int) $r['idReceita']; }
    $stmt->close();

    // IDs de ensaio deste usuário OU vinculados às receitas dele
    $idsEnsaio = [];
    $stmt = $con->prepare("SELECT idEnsaio FROM Ensaio WHERE idUsuario = ?");
    $stmt->bind_param("i", $idUsuario);
    $stmt->execute();
    $res = $stmt->get_result();
    while ($r = $res->fetch_assoc()) { $idsEnsaio[] = (int) $r['idEnsaio']; }
    $stmt->close();

    if (!empty($idsReceita)) {
        $placeholders = implode(',', array_fill(0, count($idsReceita), '?'));
        $tipos = str_repeat('i', count($idsReceita));

        $stmt = $con->prepare("SELECT idEnsaio FROM Ensaio WHERE idReceita IN ($placeholders)");
        $stmt->bind_param($tipos, ...$idsReceita);
        $stmt->execute();
        $res = $stmt->get_result();
        while ($r = $res->fetch_assoc()) { $idsEnsaio[] = (int) $r['idEnsaio']; }
        $stmt->close();
    }
    $idsEnsaio = array_values(array_unique($idsEnsaio));

    // Apaga Medida dos ensaios encontrados
    if (!empty($idsEnsaio)) {
        $placeholders = implode(',', array_fill(0, count($idsEnsaio), '?'));
        $tipos = str_repeat('i', count($idsEnsaio));
        $stmt = $con->prepare("DELETE FROM Medida WHERE idEnsaio IN ($placeholders)");
        $stmt->bind_param($tipos, ...$idsEnsaio);
        $stmt->execute();
        $stmt->close();

        $stmt = $con->prepare("DELETE FROM Ensaio WHERE idEnsaio IN ($placeholders)");
        $stmt->bind_param($tipos, ...$idsEnsaio);
        $stmt->execute();
        $stmt->close();
    }

    // Apaga Ponto e Receita
    if (!empty($idsReceita)) {
        $placeholders = implode(',', array_fill(0, count($idsReceita), '?'));
        $tipos = str_repeat('i', count($idsReceita));
        $stmt = $con->prepare("DELETE FROM Ponto WHERE idReceita IN ($placeholders)");
        $stmt->bind_param($tipos, ...$idsReceita);
        $stmt->execute();
        $stmt->close();

        $stmt = $con->prepare("DELETE FROM Receita WHERE idReceita IN ($placeholders)");
        $stmt->bind_param($tipos, ...$idsReceita);
        $stmt->execute();
        $stmt->close();
    }

    $stmt = $con->prepare("DELETE FROM Usuario WHERE idUsuario = ?");
    $stmt->bind_param("i", $idUsuario);
    if (!$stmt->execute()) {
        throw new Exception($stmt->error);
    }
    $stmt->close();

    $con->commit();
    echo json_encode([
        "success"   => true,
        "idUsuario" => $idUsuario,
        "message"   => "Usuário e todos os dados relacionados foram excluídos"
    ], JSON_UNESCAPED_UNICODE);

} catch (Exception $e) {
    $con->rollback();
    http_response_code(500);
    echo json_encode(["success" => false, "error" => $e->getMessage()], JSON_UNESCAPED_UNICODE);
}

$con->close();
?>

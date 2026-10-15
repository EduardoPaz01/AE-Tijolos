#include <Wire.h>
#include <algorithm>

// ======================================================
// CONFIGURAÇÃO
// ======================================================
#define SDA_PIN 8
#define SCL_PIN 9

// Pinos de controle (variáveis, mudam conforme o layout)
uint8_t RS = 0x01, RW = 0x02, EN = 0x04, BL = 0x08;
uint8_t layout = 0;

uint8_t lcdAddr = 0;

// pinMask[i] = bit do PCF8574 ligado a D(4+i) do LCD
uint8_t pinMask[4] = {0x10, 0x20, 0x40, 0x80};

const uint32_t FREQS[] = {10000, 50000, 100000};
int freqIdx = 1;            // começa em 50 kHz

const uint16_t EN_HIGH_US = 50;
const uint16_t EN_LOW_US  = 200;

unsigned long i2cErrors = 0;


// ======================================================
// LAYOUTS
// layout 0: dados em P4-P7, controle em P0-P3 (RS=P0 RW=P1 EN=P2 BL=P3)  <- mais comum
// layout 1: dados em P0-P3, controle em P4-P7 (RS=P4 RW=P5 EN=P6 BL=P7)
// ======================================================
void setLayout(uint8_t l)
{
    layout = l;
    if (l == 0)
    {
        RS = 0x01; RW = 0x02; EN = 0x04; BL = 0x08;
        pinMask[0] = 0x10; pinMask[1] = 0x20; pinMask[2] = 0x40; pinMask[3] = 0x80;
    }
    else
    {
        RS = 0x10; RW = 0x20; EN = 0x40; BL = 0x80;
        pinMask[0] = 0x01; pinMask[1] = 0x02; pinMask[2] = 0x04; pinMask[3] = 0x08;
    }
}

uint8_t dataMask()
{
    return pinMask[0] | pinMask[1] | pinMask[2] | pinMask[3];
}


// ======================================================
// PCF8574
// ======================================================
bool pcfWrite(uint8_t v)
{
    Wire.beginTransmission(lcdAddr);
    Wire.write(v);
    if (Wire.endTransmission() != 0)
    {
        i2cErrors++;
        return false;
    }
    return true;
}

uint8_t makeData(uint8_t n)
{
    uint8_t v = BL;
    n &= 0x0F;
    for (uint8_t i = 0; i < 4; i++)
        if (n & (1 << i))
            v |= pinMask[i];
    return v;
}

void pulseEnable(uint8_t v)
{
    pcfWrite(v | EN);
    delayMicroseconds(EN_HIGH_US);
    pcfWrite(v & ~EN);
    delayMicroseconds(EN_LOW_US);
}

void sendNibble(uint8_t n, bool rs)
{
    uint8_t v = makeData(n);
    if (rs) v |= RS;
    pulseEnable(v);
}

void sendByte(uint8_t b, bool rs)
{
    sendNibble(b >> 4, rs);
    sendNibble(b & 0x0F, rs);
}


// ======================================================
// FUNÇÕES DO LCD
// ======================================================
void lcdCommand(uint8_t c)
{
    sendByte(c, false);
    delay((c == 0x01 || c == 0x02) ? 5 : 2);
}

void lcdPrint(const char *t)
{
    while (*t)
    {
        sendByte(*t++, true);
        delayMicroseconds(100);
    }
}

void lcdSetCursor(uint8_t col, uint8_t row)
{
    lcdCommand((row ? 0xC0 : 0x80) + col);
}

void lcdInit()
{
    delay(100);
    pcfWrite(BL);
    delay(100);

    sendNibble(0x03, false); delay(10);
    sendNibble(0x03, false); delay(10);
    sendNibble(0x03, false); delay(10);
    sendNibble(0x02, false); delay(10);   // modo 4 bits

    lcdCommand(0x28);
    lcdCommand(0x08);
    lcdCommand(0x01);
    lcdCommand(0x06);
    lcdCommand(0x0C);
}

void printMap()
{
    Serial.print("Layout: "); Serial.print(layout);
    Serial.print(" | Mapa de dados: ");
    for (int i = 0; i < 4; i++)
    {
        Serial.print("D"); Serial.print(4 + i); Serial.print("=P");
        for (int p = 0; p < 8; p++)
            if (pinMask[i] == (1 << p)) Serial.print(p);
        Serial.print(i < 3 ? "  " : "\n");
    }
    Serial.print("Controle: RS=0x"); Serial.print(RS, HEX);
    Serial.print(" RW=0x");          Serial.print(RW, HEX);
    Serial.print(" EN=0x");          Serial.print(EN, HEX);
    Serial.print(" BL=0x");          Serial.println(BL, HEX);
}

void showText()
{
    i2cErrors = 0;
    lcdInit();
    lcdSetCursor(0, 0);  lcdPrint("HELLO WORLD");
    lcdSetCursor(0, 1);  lcdPrint("ESP32-S3 N16R8");

    Serial.print("Texto enviado | clock=");
    Serial.print(FREQS[freqIdx]);
    Serial.print(" Hz | NACKs=");
    Serial.println(i2cErrors);
    printMap();
}


// ======================================================
// LEITURA DE VOLTA DO LCD (RW=1)
// Obs: muitos backpacks nao ligam o RW ao PCF8574, entao este
// teste pode dar 0/40 mesmo com o LCD escrevendo certo.
// ======================================================
uint8_t readNibble(bool rs)
{
    uint8_t v = dataMask() | BL | RW | (rs ? RS : 0);   // libera linhas de dados, RW=1

    pcfWrite(v | EN);
    delayMicroseconds(100);

    Wire.requestFrom((int)lcdAddr, 1);
    uint8_t r = Wire.available() ? Wire.read() : 0xFF;

    pcfWrite(v & ~EN);
    delayMicroseconds(100);

    uint8_t n = 0;
    for (uint8_t i = 0; i < 4; i++)
        if (r & pinMask[i])
            n |= (1 << i);
    return n;
}

uint8_t readByte(bool rs)
{
    uint8_t hi = readNibble(rs);
    uint8_t lo = readNibble(rs);
    pcfWrite(BL);                    // RW volta a 0
    return (hi << 4) | lo;
}

void printBin(uint8_t v)
{
    for (int b = 7; b >= 0; b--)
        Serial.print((v >> b) & 1);
}

// Retorna pontuação (0..40). verbose=true imprime detalhes.
int lcdReadbackTest(bool verbose)
{
    int bitBad[8] = {0};

    // 1) Contador de endereço
    int ok = 0;
    for (int i = 0; i < 20; i++)
    {
        uint8_t addr = 0x05 + i;
        lcdCommand(0x80 | addr);
        uint8_t ac = readByte(false) & 0x7F;
        if (ac == addr) ok++;
        else if (verbose)
        {
            Serial.print("  AC esperado 0x"); Serial.print(addr, HEX);
            Serial.print(" veio 0x");          Serial.println(ac, HEX);
        }
    }

    // 2) Escreve caractere e lê da DDRAM
    int okc = 0;
    for (char c = 'A'; c < 'A' + 20; c++)
    {
        lcdCommand(0x80);
        sendByte(c, true);
        delayMicroseconds(100);
        lcdCommand(0x80);
        uint8_t got = readByte(true);

        if (got == (uint8_t)c) okc++;
        else
        {
            uint8_t diff = got ^ (uint8_t)c;
            for (int b = 0; b < 8; b++)
                if (diff & (1 << b)) bitBad[b]++;

            if (verbose)
            {
                Serial.print("  esperado "); printBin((uint8_t)c);
                Serial.print("  veio ");     printBin(got);
                Serial.print("  erro em: "); printBin(diff);
                Serial.println();
            }
        }
    }

    if (verbose)
    {
        Serial.print("Endereco: ");   Serial.print(ok);  Serial.println("/20 corretos");
        Serial.print("Caracteres: "); Serial.print(okc); Serial.println("/20 corretos");
        Serial.println("Erros por bit do byte (D7 ... D0):");
        Serial.print("  ");
        for (int b = 7; b >= 0; b--)
        {
            Serial.print("D"); Serial.print(b); Serial.print("="); Serial.print(bitBad[b]); Serial.print("  ");
        }
        Serial.println();
    }

    return ok + okc;
}


// ======================================================
// BUSCA DA ORDEM DOS PINOS (DESATIVADA - nao e chamada)
// Mantida so para referencia. Deu ruido nos testes.
// ======================================================
void searchPermutation()
{
    Serial.println("--- Busca da ordem dos pinos de dados (24 combinacoes) ---");

    uint8_t base = (layout == 0) ? 0x10 : 0x01;   // primeiro bit dos dados
    int idx[4] = {0, 1, 2, 3};
    int bestScore = -1;
    uint8_t best[4];
    for (int i = 0; i < 4; i++) best[i] = pinMask[i];

    do
    {
        for (int i = 0; i < 4; i++)
            pinMask[i] = base << idx[i];

        lcdInit();
        int s = lcdReadbackTest(false);

        Serial.print("  D4=bit"); Serial.print(idx[0]);
        Serial.print(" D5=bit");  Serial.print(idx[1]);
        Serial.print(" D6=bit");  Serial.print(idx[2]);
        Serial.print(" D7=bit");  Serial.print(idx[3]);
        Serial.print("  -> "); Serial.print(s); Serial.println("/40");

        if (s > bestScore)
        {
            bestScore = s;
            for (int i = 0; i < 4; i++) best[i] = pinMask[i];
        }
    } while (std::next_permutation(idx, idx + 4));

    for (int i = 0; i < 4; i++) pinMask[i] = best[i];

    Serial.print("Melhor pontuacao: "); Serial.print(bestScore); Serial.println("/40");
    printMap();
}


// ======================================================
// TESTE DE INTEGRIDADE DO BARRAMENTO (RW=0, EN=0)
// ======================================================
void integrityTest()
{
    const uint8_t pats[] = {0x00, 0xF0, 0xA0, 0x50, 0x01, 0xF1};
    int bad[8] = {0};
    int nack = 0, total = 0;

    Serial.println("Teste de integridade (RW=0, EN=0)...");

    for (int rep = 0; rep < 50; rep++)
    {
        for (uint8_t p : pats)
        {
            if (!pcfWrite(p)) { nack++; continue; }
            if (Wire.requestFrom((int)lcdAddr, 1) != 1) { nack++; continue; }

            uint8_t got = Wire.read();
            total++;

            uint8_t diff = (got ^ p) & ~BL;   // ignora o bit do backlight
            for (int b = 0; b < 8; b++)
                if (diff & (1 << b)) bad[b]++;
        }
    }

    pcfWrite(BL);

    Serial.print("Leituras: "); Serial.print(total);
    Serial.print(" | NACK: ");  Serial.println(nack);
    for (int b = 0; b < 8; b++)
    {
        if ((1 << b) == BL) continue;
        Serial.print("  P"); Serial.print(b);
        Serial.print(" divergencias: "); Serial.println(bad[b]);
    }
}


// ======================================================
// I2C SCANNER
// ======================================================
uint8_t scanI2C()
{
    uint8_t found = 0;
    for (uint8_t a = 1; a < 127; a++)
    {
        Wire.beginTransmission(a);
        if (Wire.endTransmission() == 0)
        {
            Serial.print("Dispositivo em 0x"); Serial.println(a, HEX);
            if (!found && ((a >= 0x20 && a <= 0x27) || (a >= 0x38 && a <= 0x3F)))
                found = a;
        }
    }
    return found;
}


// ======================================================
// SETUP / LOOP
// ======================================================
void setup()
{
    Serial.begin(115200);
    unsigned long t0 = millis();
    while (!Serial && millis() - t0 < 3000) delay(10);
    delay(500);

    Serial.println("\n=== TESTE LCD 1602 I2C ===");
    Serial.println("Comandos: t=barramento  x=leitura do LCD");
    Serial.println("          f=troca clock  m=inverte ordem dos dados");
    Serial.println("          d=troca layout (0/1)  r=reenvia texto");

    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(FREQS[freqIdx]);
    Wire.setTimeOut(100);

    setLayout(0);   // mapa padrao

    lcdAddr = scanI2C();
    if (!lcdAddr)
    {
        Serial.println("ERRO: PCF8574 nao encontrado.");
        while (true) delay(1000);
    }
    Serial.print("Usando 0x"); Serial.println(lcdAddr, HEX);

    integrityTest();

    // Busca de permutacao DESATIVADA:
    // searchPermutation();

    showText();
}

void loop()
{
    while (Serial.available())
    {
        char c = Serial.read();

        if (c == 't')
            integrityTest();
        else if (c == 'x')
        {
            lcdInit();
            lcdReadbackTest(true);
            showText();
        }
        // else if (c == 'p')
        // {
        //     searchPermutation();
        //     showText();
        // }
        else if (c == 'f')
        {
            freqIdx = (freqIdx + 1) % 3;
            Wire.setClock(FREQS[freqIdx]);
            showText();
        }
        else if (c == 'm')
        {
            std::swap(pinMask[0], pinMask[3]);
            std::swap(pinMask[1], pinMask[2]);
            showText();
        }
        else if (c == 'd')
        {
            setLayout((layout + 1) % 2);   // volta ao mapa padrao do novo layout
            showText();
        }
        else if (c == 'r')
            showText();
    }
}
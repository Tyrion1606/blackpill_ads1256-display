#!/usr/bin/env python3

import csv
import time
import serial
from datetime import datetime

PORTA = "/dev/ttyACM1"
BAUDRATE = 115200
DURACAO_SEGUNDOS = 10
ARQUIVO_SAIDA = "captura_serial.csv"

COLUNAS = [
    "tempo_pc_iso",
    "tempo_pc_s",
    "FRAME",
    "SEQ",
    "TIME",
    "D1",
    "D2",
    "D3",
    "D4",
]


def main():
    inicio = time.time()
    linhas_validas = 0
    linhas_invalidas = 0

    with serial.Serial(PORTA, BAUDRATE, timeout=1) as ser, \
         open(ARQUIVO_SAIDA, "w", newline="", encoding="utf-8") as f:

        writer = csv.writer(f)
        writer.writerow(COLUNAS)

        print(f"Lendo {PORTA} a {BAUDRATE} por {DURACAO_SEGUNDOS} segundos...")
        print(f"Salvando em: {ARQUIVO_SAIDA}")

        while time.time() - inicio < DURACAO_SEGUNDOS:
            bruto = ser.readline()

            if not bruto:
                continue

            linha = bruto.decode("utf-8", errors="replace").strip()

            if not linha:
                continue

            agora = time.time()
            tempo_pc_iso = datetime.now().isoformat(timespec="milliseconds")
            tempo_pc_s = agora - inicio

            partes = linha.split(",")

            if len(partes) == 7 and partes[0] == "FRAME":
                writer.writerow([
                    tempo_pc_iso,
                    f"{tempo_pc_s:.6f}",
                    partes[0],
                    partes[1],
                    partes[2],
                    partes[3],
                    partes[4],
                    partes[5],
                    partes[6],
                ])

                linhas_validas += 1
                print(linha)
            else:
                linhas_invalidas += 1
                print(f"Linha ignorada: {linha}")

    print("Finalizado.")
    print(f"Linhas válidas gravadas: {linhas_validas}")
    print(f"Linhas inválidas ignoradas: {linhas_invalidas}")


if __name__ == "__main__":
    main()
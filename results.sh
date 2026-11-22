#!/bin/sh
# Lee lo que deja main.c en la variable "results" y lo saca como tabla.
# Uso: results.sh main.elf [nivel de optimizacion]
#
# Con FAKE_DUMP=fichero lee el volcado de ese fichero en vez de la placa
# (para probar el script sin tenerla conectada).

ELF=${1:-main.elf}
OPT=${2:-?}
NM=${NM:-arm-none-eabi-nm}
WORDS=19        # 3 + 4 versiones * 2 tamanos * (ciclos + checksum)

ADDR=$($NM "$ELF" | awk '$3 == "results" { print $1 }')
if [ -z "$ADDR" ]; then
  echo "results.sh: no encuentro el simbolo 'results' en $ELF" >&2
  exit 1
fi

if [ -n "$FAKE_DUMP" ]; then
  DUMP=$(cat "$FAKE_DUMP")
else
  # Reset, esperar a que acabe el benchmark, parar y volcar la variable.
  # Lo que devuelve un comando pasado con -c no se imprime, de ahi el
  # echo [capture ...]
  DUMP=$(openocd -f openocd.cfg \
    -c "init; reset run; sleep 2000; halt" \
    -c "echo [capture \"mdw 0x$ADDR $WORDS\"]" \
    -c "resume; exit" 2>&1)
fi

echo "$DUMP" | awk -v opt="$OPT" '
  # Hexadecimal a numero (strtonum no esta en todos los awk)
  function hex(s,    i, v) {
    v = 0
    for (i = 1; i <= length(s); i++)
      v = v * 16 + index("0123456789abcdef", tolower(substr(s, i, 1))) - 1
    return v
  }
  { sub(/[ \r]+$/, "") }
  /^0x[0-9a-fA-F]+: / {
    for (i = 2; i <= NF; i++)
      w[n++] = $i
  }
  END {
    if (n < 19) {
      print "results.sh: no se han podido leer los resultados de la placa" > "/dev/stderr"
      exit 1
    }
    if (w[0] != "f16c0de5") {
      print "results.sh: el benchmark no ha terminado (done = 0x" w[0] ")" > "/dev/stderr"
      exit 1
    }
    name[0] = "C naive"
    name[1] = "C resta condicional"
    name[2] = "C modulo diferido"
    name[3] = "asm modulo diferido x8"

    printf "Fletcher-16, ciclos medidos con el SysTick (%s)\n\n", opt
    printf "%-28s %10s %10s   %s\n", "Rutina", "N=100", "N=1024", "checksums"
    for (v = 0; v < 4; v++)
      printf "%-28s %10d %10d   0x%s 0x%s\n", name[v],
             hex(w[3 + 2 * v]), hex(w[4 + 2 * v]),
             substr(w[11 + 2 * v], 5), substr(w[12 + 2 * v], 5)
    printf "\nCoste de la medida: %d ciclos\n", hex(w[2])
    if (w[1] != "00000000") {
      printf "ERROR: checksums incorrectos, mascara de versiones 0x%s\n", w[1]
      exit 1
    }
    print "Todos los checksums OK"
  }'

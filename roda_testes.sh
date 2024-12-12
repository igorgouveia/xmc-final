#!/bin/bash

# Verifica se ambiente virtual existe
if [ ! -d "venv" ]; then
    echo "Ambiente virtual não encontrado. Execute ./install_deps.sh primeiro"
    exit 1
fi

# Ativa ambiente virtual
source venv/bin/activate

# Limpa diretórios anteriores
rm -rf logs resultados
mkdir -p logs resultados

# Compila
make clean
make

# Executa testes
echo "Executando testes..."

# Testes pequenos e médios
echo "Testando instâncias pequenas..."
for i in {1..3}; do
    ./tsp_bb instances/small_$i.txt
    ./tsp_mip instances/small_$i.txt
done

echo "Testando instâncias médias..."
for i in {1..3}; do
    ./tsp_bb instances/medium_$i.txt
    ./tsp_mip instances/medium_$i.txt
done

# Testes grandes apenas se especificado
if [ "$1" = "--all" ]; then
    echo "Testando instâncias grandes..."
    for i in {1..3}; do
        ./tsp_bb instances/large_$i.txt
        ./tsp_mip instances/large_$i.txt
    done
fi

# Gera resultados
echo "Gerando resultados..."
python3 scripts/gera_tabelas.py
python3 scripts/gera_graficos.py

# Gera análise comparativa
echo "Gerando análise comparativa..."
python3 scripts/analise_comparativa.py

# Desativa ambiente virtual
deactivate

echo "Processo concluído!"
echo "Resultados disponíveis em:"
echo "- Logs: diretório logs/"
echo "- Tabelas: resultados/tabela{1,2}.{tex,csv}"
echo "- Gráficos: resultados/comparacao_{tempos,gaps}.png"
echo "- Análise: resultados/analise_comparativa.txt"
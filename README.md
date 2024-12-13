# TSP: Branch and Bound vs PLI
**Autor:** Igor Oliveira

## Descrição
Implementação e comparação de dois métodos para o Problema do Caixeiro Viajante (TSP):
- Branch and Bound (BB)
- Programação Linear Inteira (PLI)

## Instalação Mínima (Apenas C)

### macOS
```bash
brew install gcc glpk
```

### Linux (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install -y build-essential glpk-utils libglpk-dev
```

### Compilar
```bash
make clean
make
```

## Uso Básico

### Executar uma Instância
```bash
./tsp_bb instances/arquivo.txt  # Branch and Bound
./tsp_mip instances/arquivo.txt  # PLI
```
Os resultados serão salvos em `logs/arquivo_BB.log` ou `logs/arquivo_PLI.log`

### Formato do Arquivo de Entrada
```
n_cidades
cidade_1 tempo_minimo_1
cidade_2 tempo_minimo_2
...
cidade_n tempo_minimo_n
[matriz_distancias n x n]
[matriz_riscos n x n]
```

### Exemplo (small_1.txt)
```
3
KingsLanding 2
Winterfell 3
CasterlyRock 4
0 100 150
100 0 180
150 180 0
0.0 0.2 0.3
0.2 0.0 0.3
0.3 0.3 0.0
```

## Análises Adicionais (Opcional)

Se desejar gerar gráficos e análises comparativas:

### Instalar Python e Dependências
```bash
brew install python3  # macOS
sudo apt install python3-pip  # Linux

./install_deps.sh
```

### Executar Análises
```bash
./roda_testes.sh  # Roda todos os testes small e medium e gera análises
```

```bash
./roda_testes.sh  --all # Roda todos os testes small, medium e large e gera análises
```
### Resultados
- `logs/`: Logs detalhados de cada execução
- `resultados/`: Tabelas e gráficos (requer Python)
- `doc_result.md`: Análise completa (requer Python)

## Referências
- [GLPK Documentation](https://www.gnu.org/software/glpk/)
import os
import pandas as pd

def processa_logs():
    resultados = []
    
    for arquivo in os.listdir('logs'):
        if arquivo.endswith('.log'):
            with open(f'logs/{arquivo}', 'r') as f:
                conteudo = f.read()
                linhas = conteudo.split('\n')
                
                dados = {}
                resultados_finais = False
                
                for linha in linhas:
                    if 'Instância:' in linha and not 'Resultados finais' in linha:
                        nome_instancia = linha.split(':')[1].strip()
                        nome_instancia = nome_instancia.replace('.txt', '')
                        dados['Instância'] = nome_instancia
                    elif 'Método:' in linha:
                        dados['Método'] = 'BB' if 'Branch' in linha else 'PLI'
                    elif 'Resultados finais' in linha:
                        resultados_finais = True
                    elif resultados_finais and 'Custo:' in linha:
                        dados['Custo'] = float(linha.split(':')[1].strip())
                    elif resultados_finais and 'Gap:' in linha:
                        dados['Gap (%)'] = float(linha.split(':')[1].strip().replace('%',''))
                    elif resultados_finais and 'Tempo:' in linha:
                        dados['Tempo (s)'] = float(linha.split(':')[1].strip().replace('s','').strip())
                
                if len(dados) == 5:  # Todos os campos necessários
                    resultados.append(dados)
                    print(f"Processado {arquivo}: {dados}")
                else:
                    print(f"AVISO: Arquivo {arquivo} não contém todos os campos necessários")
                    print(f"Campos encontrados: {dados.keys()}")
                    print("Conteúdo do arquivo:")
                    print("---")
                    print(conteudo)
                    print("---")
    
    if not resultados:
        print("AVISO: Nenhum resultado processado!")
        return pd.DataFrame()
        
    df = pd.DataFrame(resultados)
    
    # Ordena as instâncias
    df['Tipo'] = df['Instância'].apply(lambda x: x.split('_')[0])
    df['Numero'] = df['Instância'].apply(lambda x: int(x.split('_')[1]))
    df = df.sort_values(['Tipo', 'Numero'])
    
    return df[['Instância', 'Método', 'Custo', 'Gap (%)', 'Tempo (s)']]

def gera_tabelas():
    if not os.path.exists('resultados'):
        os.makedirs('resultados')
        
    df = processa_logs()
    
    if df.empty:
        print("Nenhum resultado encontrado nos logs!")
        return
        
    # Tabela 1: Resultados por tamanho de instância
    tabela1 = df.pivot_table(
        index=['Instância'],
        columns=['Método'],
        values=['Custo', 'Gap (%)', 'Tempo (s)'],
        aggfunc='mean'
    )
    
    # Tabela 2: Comparação BB vs PLI
    tabela2 = df.groupby('Método').agg({
        'Custo': ['mean', 'std'],
        'Gap (%)': ['mean', 'std'],
        'Tempo (s)': ['mean', 'std']
    })
    
    # Prepara dados para os gráficos
    df_graficos = pd.DataFrame()
    for metodo in ['BB', 'PLI']:
        df_metodo = df[df['Método'] == metodo].copy()
        df_metodo.set_index('Instância', inplace=True)
        for coluna in ['Custo', 'Gap (%)', 'Tempo (s)']:
            df_graficos[f'{coluna}_{metodo}'] = df_metodo[coluna]
    
    df_graficos.reset_index(inplace=True)
    
    # Salva tabelas
    try:
        tabela1.to_latex('resultados/tabela1.tex')
        tabela2.to_latex('resultados/tabela2.tex')
    except ImportError:
        with open('resultados/tabela1.tex', 'w') as f:
            f.write(tabela1.to_string())
        with open('resultados/tabela2.tex', 'w') as f:
            f.write(tabela2.to_string())
    
    # Salva CSV no formato correto para os gráficos
    df_graficos.to_csv('resultados/tabela1.csv', index=False)
    tabela2.to_csv('resultados/tabela2.csv')
    
    print("\nTabelas geradas:")
    print("\nResultados por instância:")
    print(tabela1)
    print("\nEstatísticas por método:")
    print(tabela2)

if __name__ == '__main__':
    gera_tabelas() 
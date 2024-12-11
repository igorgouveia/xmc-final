import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os

def compara_tempos(df):
    """Analisa qual método é mais rápido para instâncias resolvidas por ambos"""
    # Filtra apenas instâncias resolvidas por ambos
    df_completo = df[(df['Custo_BB'] > 0) & (df['Custo_PLI'] > 0)]
    
    print("\n=== Comparação de Tempos ===")
    print("Instâncias resolvidas por ambos:")
    for _, row in df_completo.iterrows():
        print(f"\n{row['Instância']}:")
        print(f"  BB: {row['Tempo (s)_BB']:.2f}s")
        print(f"  PLI: {row['Tempo (s)_PLI']:.2f}s")
    
    print("\nMédias:")
    print(f"BB: {df_completo['Tempo (s)_BB'].mean():.2f}s")
    print(f"PLI: {df_completo['Tempo (s)_PLI'].mean():.2f}s")

def analisa_limite_tempo(df):
    """Analisa maiores instâncias resolvidas em 600s"""
    print("\n=== Limite de Tempo (600s) ===")
    
    # Considera resolvida se tempo < 600s e solução viável
    bb_resolvidas = df[df['Tempo (s)_BB'] < 600]['Instância'].tolist()
    pli_resolvidas = df[df['Tempo (s)_PLI'] < 600]['Instância'].tolist()
    
    print("\nInstâncias resolvidas BB:")
    for inst in sorted(bb_resolvidas):
        row = df[df['Instância'] == inst].iloc[0]
        print(f"  {inst}: {row['Tempo (s)_BB']:.2f}s")
        
    print("\nInstâncias resolvidas PLI:")
    for inst in sorted(pli_resolvidas):
        row = df[df['Instância'] == inst].iloc[0]
        print(f"  {inst}: {row['Tempo (s)_PLI']:.2f}s")

def analisa_viabilidade(df):
    """Analisa soluções viáveis após timeout"""
    print("\n=== Viabilidade após 600s ===")
    
    # Analisa casos que excederam tempo
    bb_timeout = df[df['Tempo (s)_BB'] >= 600]
    pli_timeout = df[df['Tempo (s)_PLI'] >= 600]
    
    print("\nBranch and Bound:")
    for _, row in bb_timeout.iterrows():
        print(f"  {row['Instância']}: {'Viável' if row['Custo_BB'] > 0 else 'Inviável'}")
    
    print("\nPLI:")
    for _, row in pli_timeout.iterrows():
        print(f"  {row['Instância']}: {'Viável' if row['Custo_PLI'] > 0 else 'Inviável'}")

def compara_solucoes(df):
    """Compara qualidade das soluções não-ótimas"""
    print("\n=== Comparação de Soluções ===")
    
    # Filtra casos não-ótimos (gap > 0)
    nao_otimas = df[(df['Gap (%)_BB'] > 0) | (df['Gap (%)_PLI'] > 0)]
    
    for _, row in nao_otimas.iterrows():
        print(f"\n{row['Instância']}:")
        print(f"  BB: Custo={row['Custo_BB']:.2f}, Gap={row['Gap (%)_BB']:.2f}%")
        print(f"  PLI: Custo={row['Custo_PLI']:.2f}, Gap={row['Gap (%)_PLI']:.2f}%")
        
        if row['Custo_BB'] > 0 and row['Custo_PLI'] > 0:
            diff = abs(row['Custo_BB'] - row['Custo_PLI'])
            pct = (diff / min(row['Custo_BB'], row['Custo_PLI'])) * 100
            print(f"  Diferença: {diff:.2f} ({pct:.2f}%)")

def gera_graficos_comparativos(df):
    """Gera gráficos comparativos"""
    if not os.path.exists('resultados'):
        os.makedirs('resultados')
    
    # Configura estilo
    plt.style.use('default')
    sns.set_theme(style="whitegrid")
    
    # Gráfico de tempos
    plt.figure(figsize=(12, 6))
    df_melted = pd.melt(df, 
                        id_vars=['Instância'],
                        value_vars=['Tempo (s)_BB', 'Tempo (s)_PLI'],
                        var_name='Método',
                        value_name='Tempo (s)')
    sns.barplot(data=df_melted, x='Instância', y='Tempo (s)', hue='Método')
    plt.title('Comparação de Tempos de Execução')
    plt.xticks(rotation=45)
    plt.tight_layout()
    plt.savefig('resultados/comparacao_tempos.png')
    plt.close()
    
    # Gráfico de gaps
    plt.figure(figsize=(12, 6))
    df_melted = pd.melt(df,
                        id_vars=['Instância'],
                        value_vars=['Gap (%)_BB', 'Gap (%)_PLI'],
                        var_name='Método',
                        value_name='Gap (%)')
    sns.barplot(data=df_melted, x='Instância', y='Gap (%)', hue='Método')
    plt.title('Comparação de Gaps de Otimalidade')
    plt.xticks(rotation=45)
    plt.tight_layout()
    plt.savefig('resultados/comparacao_gaps.png')
    plt.close()

def main():
    # Lê dados
    df = pd.read_csv('resultados/tabela1.csv')
    
    # Imprime estrutura dos dados para debug
    print("Colunas disponíveis:")
    print(df.columns.tolist())
    print("\nPrimeiras linhas:")
    print(df.head())
    
    # Gera relatório
    with open('resultados/analise_comparativa.txt', 'w') as f:
        # Redireciona stdout para o arquivo
        import sys
        stdout = sys.stdout
        sys.stdout = f
        
        print("=== Análise Comparativa BB vs PLI ===")
        print("Autor: Igor Oliveira")
        print("Data:", pd.Timestamp.now().strftime("%Y-%m-%d %H:%M:%S"))
        
        compara_tempos(df)
        analisa_limite_tempo(df)
        analisa_viabilidade(df)
        compara_solucoes(df)
        
        sys.stdout = stdout
    
    # Gera gráficos
    gera_graficos_comparativos(df)
    
    print("Análise completa gerada em:")
    print("- resultados/analise_comparativa.txt")
    print("- resultados/comparacao_tempos.png")
    print("- resultados/comparacao_gaps.png")

if __name__ == '__main__':
    main() 
import pandas as pd

def compara_tempos():
    df = pd.read_csv('resultados/tabela1.csv')
    
    # Filtra apenas instâncias resolvidas por ambos
    df_completo = df[df['Custo_BB'] > 0 & df['Custo_PLI'] > 0]
    
    print("Comparação de tempos para instâncias resolvidas por ambos:")
    print(f"Tempo médio BB: {df_completo['Tempo_BB'].mean():.2f}s")
    print(f"Tempo médio PLI: {df_completo['Tempo_PLI'].mean():.2f}s") 
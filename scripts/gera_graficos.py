import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns
import os

def gera_graficos():
    if not os.path.exists('resultados/tabela1.csv'):
        print("Arquivo tabela1.csv não encontrado!")
        return
        
    # Lê os dados
    df = pd.read_csv('resultados/tabela1.csv')
    
    # Configura estilo
    plt.style.use('default')
    sns.set_theme(style="whitegrid")
    
    # Configurações comuns
    plt.rcParams['figure.figsize'] = (12, 6)
    plt.rcParams['font.size'] = 10
    plt.rcParams['axes.labelsize'] = 12
    plt.rcParams['axes.titlesize'] = 14
    
    # Prepara dados para os gráficos
    plot_data = pd.DataFrame()
    plot_data['Instância'] = df['Instância']
    
    # Gráfico 1: Comparação de custos
    plt.figure()
    plot_data['BB'] = df['Custo_BB']
    plot_data['PLI'] = df['Custo_PLI']
    plot_data_melted = plot_data.melt(id_vars=['Instância'], 
                                     var_name='Método',
                                     value_name='Custo')
    
    ax = sns.barplot(data=plot_data_melted, x='Instância', y='Custo', hue='Método')
    plt.title('Comparação de Custos por Método')
    plt.ylabel('Custo')
    plt.xticks(rotation=45)
    plt.legend(title='Método')
    plt.tight_layout()
    plt.savefig('resultados/custos.png', dpi=300, bbox_inches='tight')
    plt.close()
    
    # Gráfico 2: Comparação de gaps
    plt.figure()
    plot_data['BB'] = df['Gap (%)_BB']
    plot_data['PLI'] = df['Gap (%)_PLI']
    plot_data_melted = plot_data.melt(id_vars=['Instância'], 
                                     var_name='Método',
                                     value_name='Gap (%)')
    
    ax = sns.barplot(data=plot_data_melted, x='Instância', y='Gap (%)', hue='Método')
    plt.title('Comparação de Gaps por Método')
    plt.ylabel('Gap (%)')
    plt.xticks(rotation=45)
    plt.legend(title='Método')
    plt.tight_layout()
    plt.savefig('resultados/gaps.png', dpi=300, bbox_inches='tight')
    plt.close()
    
    # Gráfico 3: Comparação de tempos
    plt.figure()
    plot_data['BB'] = df['Tempo (s)_BB']
    plot_data['PLI'] = df['Tempo (s)_PLI']
    plot_data_melted = plot_data.melt(id_vars=['Instância'], 
                                     var_name='Método',
                                     value_name='Tempo (s)')
    
    ax = sns.barplot(data=plot_data_melted, x='Instância', y='Tempo (s)', hue='Método')
    plt.title('Comparação de Tempos por Método')
    plt.ylabel('Tempo (s)')
    plt.xticks(rotation=45)
    plt.legend(title='Método')
    plt.tight_layout()
    plt.savefig('resultados/tempos.png', dpi=300, bbox_inches='tight')
    plt.close()
    
    print("Gráficos gerados com sucesso!")

if __name__ == '__main__':
    gera_graficos() 
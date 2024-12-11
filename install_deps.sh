#!/bin/bash

echo "Instalando dependências..."

# Verifica se pip está instalado
if ! command -v pip3 &> /dev/null; then
    echo "Instalando pip3..."
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS
        brew install python3
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        # Linux
        sudo apt-get update
        sudo apt-get install -y python3-pip
    fi
fi

# Instala virtualenv
echo "Instalando virtualenv..."
pip3 install virtualenv

# Remove ambiente virtual anterior se existir
rm -rf venv

# Cria ambiente virtual
echo "Criando ambiente virtual..."
python3 -m venv venv

# Ativa ambiente virtual
echo "Ativando ambiente virtual..."
source venv/bin/activate

# Instala dependências Python
echo "Instalando dependências Python..."
pip install pandas matplotlib seaborn jinja2 tabulate

# Desativa ambiente virtual
deactivate

echo "Dependências instaladas com sucesso!" 
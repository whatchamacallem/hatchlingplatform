

python3 -m venv python_venv

source python_venv/bin/activate

pip install .

python -c "import entanglement_test; entanglement_test.ping();"

deactivate

rm -rf python_venv

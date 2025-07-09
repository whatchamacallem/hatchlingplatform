

python3 -m venv python_venv

source python_venv/bin/activate

pip install .

python -c "import entanglement_template; entanglement_template.run_all_tests();"

deactivate

rm -rf python_venv __pycache__ build entanglement_template.egg-info

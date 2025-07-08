

python3 -m venv python_venv

source python_venv/bin/activate

pip install .

python -c "import entanglement_test; entanglement_test.run_all_tests();"

deactivate

rm -rf python_venv __pycache__ entanglement_test.egg-info

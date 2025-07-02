

python3 -m venv temp_venv

source temp_venv/bin/activate

pip install .

python -c "import example; example.greet(25.0); example.greet(100.0);"

deactivate

rm -rf temp_venv build example_package.egg-info __pycache__

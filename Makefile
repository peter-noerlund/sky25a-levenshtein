all: fpga

fpga: tt/tt_tool.py venv/bin/activate
	. venv/bin/activate && tt/tt_tool.py --create-fpga-bitstream

harden: src/user_config.json
	. venv/bin/activate && tt/tt_tool.py --openlane2 --harden

src/user_config.json: src/config.json
	. venv/bin/activate && tt/tt_tool.py --openlane2 --create-user-config

test: venv/bin/activate
	. venv/bin/activate && $(MAKE) -C test

tt/tt_tool.py:
	git clone -b tt09 https://github.com/TinyTapeout/tt-support-tools tt

venv/bin/activate:
	python3 -m venv venv
	./venv/bin/pip install -r tt/requirements.txt

.PHONY: all fpga test
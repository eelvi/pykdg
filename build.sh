     mkdir virenv \
 &&  python3 -m venv virenv/ \
 &&  source virenv/bin/activate \
 &&  cd kdg \
 &&  make \
 &&  cd ../pykdgsetup \
 &&  python3 setup.py build \
 &&  python3 setup.py install \


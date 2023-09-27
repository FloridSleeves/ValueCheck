sudo apt install cmake gcc g++ libtinfo5 libz-dev zip wget 
cd SVF/
source build.sh
cd ..
source SVF/setup.sh
cd valuecheck/
mkdir -p build
cd build
cmake ..
make

python3 -m venv vc-env
source vc-env/bin/activate
pip install cxxfilt gitpython numpy matplotlib

git clone https://github.com/torvalds/linux.git
git clone https://github.com/mysql/mysql-server.git
git clone https://github.com/nfs-ganesha/nfs-ganesha.git
git clone https://github.com/openssl/openssl.git

cd linux
git checkout v5.19
cd ../mysql-server
git checkout mysql-8.0.30
cd ../nfs-ganesha
git checkout V4-dev.46
cd ../openssl
git checkout ea08f8b294d129371536649463c76a81dc4d4e55
cd ..

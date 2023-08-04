rm bin/csl*
make xz
cp ../csl.tar.xz ../csl.$1.tar.xz
cp ../csl.$1.tar.xz ../backup/csl.$1.tar.xz
mv ../csl.$1.tar.xz ../backup/archive/csl.$1.tar.xz
ls -al ../backup/csl.$1.tar.xz
ls -al ../backup/archive/csl.$1.tar.xz

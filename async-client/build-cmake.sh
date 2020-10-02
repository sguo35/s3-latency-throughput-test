if [ -z $(which cmake3) ]
then
    cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release .
else
    cmake3 -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release .
fi

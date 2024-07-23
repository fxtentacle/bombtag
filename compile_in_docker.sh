docker build -t "bombtag" .
docker run --name=bombtag "bombtag"
docker cp bombtag:/root/build/bombtag ./bombtag
docker rm -f bombtag

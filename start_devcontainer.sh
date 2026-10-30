if [ -f /.dockerenv ]; then
    echo "You are already in a Docker container. Aborting."
    exit 1
fi

docker build -t ahoiplank-dev .
docker run -it --rm -v $(pwd):/workspace -w /workspace --user $(id -u):$(id -g) ahoiplank-dev

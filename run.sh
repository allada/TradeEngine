docker build --tag trader .;

docker rm trader;

docker create --name trader trader;

docker run -i -t trader bash
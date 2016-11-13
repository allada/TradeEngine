bool hasData()
{
    
}

size_t receive(char* data, size_t len)
{
    return recv(sock_, data, len, 0);
}
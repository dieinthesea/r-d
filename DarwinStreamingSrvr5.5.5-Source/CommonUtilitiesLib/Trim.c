char* TrimLeft(char* fromStrPtr )
{
    char* tmp = &fromStrPtr[0];
    // trim any leading white space
    while ( (*tmp <= ' ') && (*tmp != 0) )
        tmp++;
    return tmp;
}

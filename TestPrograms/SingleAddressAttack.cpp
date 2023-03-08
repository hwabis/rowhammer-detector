#define FLIP_COUNT 100000

int main()
{
    int a = 0;
    for (int i = 0; i < FLIP_COUNT; i++)
    {
        if (a == 0)
            a = -1;
        else
            a = 0;
    }
}
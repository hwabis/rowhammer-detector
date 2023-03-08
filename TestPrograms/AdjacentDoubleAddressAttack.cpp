#define FLIP_COUNT 100000

int main()
{
    int a[2] = {0};
    for (int i = 0; i < FLIP_COUNT; i++)
    {
        if (a[0] == 0)
            a[0] = -1;
        else
            a[0] = 0;

        if (a[1] == 0)
            a[1] = -1;
        else
            a[1] = 0;
    }
}
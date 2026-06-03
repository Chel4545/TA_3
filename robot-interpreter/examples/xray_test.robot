func countdown(signed n) (
    testonce (n > 0) (
        signed current <- n;
        call countdown(n - 1);
    )
)

func start() (
    call countdown(3);
)
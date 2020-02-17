LATHE!

Hello,
What you need is a computer with a native MS-DOS running, LPT socket and 2 Stepper motors.

What is alread done and working :
That program support NEMA and  28BYJ-48  stepper motor.
NEMA works with 4 states meanwhile  28BYJ-48  with 7 states.
The program right now allow to order the 2 step mottors (X and Y) to move back/forward for how many steps.
Then can you figure out to progress in the screw (each complete spin = +1 pass in the screw).
Also it is possible interrupt the process at any time with an emergency real button that connect the pin11 of the LPT to the ground.

What we would add in future:
Working in progress in a graphic feedback for a simulation before launch the program
and put more details like the RPM of the drill (that spin the pics) and basically the program
should store in memory the dimension of volume of the pics (is it a cilinder? ).

Thanks for the reading
If you want support us feel to free to contact us!

That project is figured as open source right now.

![lathe-lpt-interface-msdos](/structure.png)


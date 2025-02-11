# YSpice

Or more like YOLO spice.

Put everything so that you get a resistor network.

Solve using newton-rapson or euler's method or Runge-Kutta.

1. Find islands of potential.  I.e. everything else separated by either inductors or resistors.
2. Lock island of potential.
3. Work one island at a time.
4. Find new potential of each island.
5. Find current across all resistors.
5. Find current across all capacitors.
6. Modify potential across capacitors.
7. Go back to step 2.

Need:
1. What NETs constitue an island?
2. Have a island leader (that other nets voltage deltas in the island are computed from).
3. The voltage deltas of each in-island net.  Or more, each NET needs a delta to its island leader and WHO that island leader is.

NET:
 * Who is my island leader?
 * What is my delta voltage to it?
 * What other NETs are this island connected to via resistors?



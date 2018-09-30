### Errata zu _GPU-beschleunigte Schnittvolumenberechnung auf Dreiecksnetzen_

#### Seite 32 - _Abbildung 4.5._

- `L.s` muss entgegen der Normalenrichtung von `T2` liegen, um sich innerhalb
  des Schnittes zu befinden, das Skalarprodukt im Pseudocode muss dazu auf
  Negativität geprüft werden.

- Wenn der Endpunkt einer Kante ein innerer Punkt ist, gilt nicht
  `ins xor co mod 2 = 0`, sondern `ins xor co mod 2 = 1`.

Beide Errata wurden in der Implementierung bereits korrigiert.

#### Seite 36 - _4.6.4. Ungelöste Probleme_

Effiziente Breitensuche wurde bereits [erfolgreich auf GPUs umgesetzt][nvidia-dfs],
die Auffindung der nicht durch Kantenschnitte bereits klassifizierten Punkte könnte also
entgegen der in der Arbeit getroffenen Annahme ebenfalls von GPU-Beschleunigung
profitieren.

[nvidia-dfs]: https://research.nvidia.com/publication/scalable-gpu-graph-traversal

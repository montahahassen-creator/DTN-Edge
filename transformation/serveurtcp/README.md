# Socket TCP en C++ sous Linux

```
          CLIENT                                   SERVEUR

        +----------+                            +----------+
        | socket() |      Creation socket       | socket() |
        +----------+                            +----------+
             |                                       |
             |                                  +----------+
             |                                  |  bind()  |  Association
             |                                  +----------+  @IP + N° port
             |                                       |
             |                                  +----------+
             |                                  | listen() |  Ecoute des
             |                                  +----------+  connexions entrantes
             |                                       |
        +----------+                            +----------+
        |connect() | -------------------------> |[accept()]|
        +----------+  Etablissement connexion   +----------+
             |                                       |
        +----------+                            +----------+
        |  send()  | -------------------------> | [recv()] |
        +----------+   Transfert des donnees    +----------+
             |                                       |
        +----------+                            +----------+
        | [recv()] | <------------------------- |  send()  |
        +----------+                            +----------+
             |                                       |
        +----------+                            +----------+
        | close()  |                            | close()  |
        +----------+                            +----------+

        [ ... ]  Pouvant etre bloquant
        |     |  Appel non bloquant
```

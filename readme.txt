// Drastaru Florina Cristina 325CA

Tema 2 - Protocoale de comunicatii

STRUCTURA TEMA:
    - server.cpp
    - subscriber.cpp
    - structures.h
    - helpers.h
    - Makefile
    - readme.txt

REFERINTE

Am realizat tema pornind de la laboratoarele 6, 7 si 8.
Pot spune ca am folosit laboratorul 8 ca si schelet, adaptandu-l insa pentru mai multe 
conexiuni si pentru un mod diferit de rulare a clientului(id-ul face parte din comanda).
De asemenea, si Makefile l-am preluat tot din lab 8, modificandu-l pentru sursa cpp.

STRUCTURES
Am creat:
    - o structura 'subscription' in care salvez numele topicului si valoarea lui sf;
    - o structura 'client' in care tin minte id-ul, daca este online sau nu, subscriptiile,
        socketul pe care se conecteaza si mesajele pe care le primeste la reconectare
        daca sf = 1;
    - o structura pentru mesaj trimis de la clientul tcp catre server cu topic, sf
    si comanda(1 pt subscribe si 0 pt unsubscribe);
    - o structura pentru mesaj trimis de la clientul udp serverului cu topic, tip de date
    si continut;
    - o structura pentru mesaj trimis de la server catre clientii tcp cu ip, port, topic, 
    tip de date, continut


SUBSCRIBER - implementare

Initializez socketul de tcp, adaug datele necesare pt conexiunea cu serverul, 
golesc multimea de descriptori, apoi setez file descriptorii.

Primul parametru al comenzii este id-ul clientului, asa ca il trimit la server.
Citesc de la tastatura comanda si impart bufferul in tokenuri pentru a verifica
fiecare parametru si a-l salva in mesajul ce trebuie trimis catre server.
Dupa ce trimit mesajul, in functie de comanda, afisez ce a facut clientul legat
de topicul respectiv(subscribed/unsubscribed).

In cazul in care primesc mesaj de la server, convertesc bufferul in structura 
specifica si il afisez. Pentru aceasta, verific tipul de date primit ca int si
afisez stringul corespunzator.

SERVER - implementare

Initializez socketii de tcp si udp si completez cu toate datele necesare pentru
conexiuni. Apoi, verific daca socketul pe care ma aflu este cel de tcp sau de udp, 
daca se realizeaza o conexiune noua, sau daca primesc comanda exit.

In cazul socketului de udp, voi primi de la clientul udp mesaje(pe care mi le 
stochez intr-o structura specifica). In functie de tipul de date din mesaj, 
convertesc continutul din format binar dupa regulile din enuntul temei.
Dupa aceea, verific vectorul de clienti si, verificand subscriptiile fiecarui 
client, ii caut pe aceia care sunt abonati la topicul primit in mesaj de la 
clientul udp. Daca clientii gasiti sunt online, trimit mesajul. Daca clientul
gasit nu este online, insa sf este egal cu 1(adica aplic store&forwarding), 
salvez mesajul respectiv intr-o coada.

In cazul socketului de tcp, voi avea un socket nou(deci o conexiune noua), 
pe care il adaug la file descriptori. In continuare, primesc id-ul trimis 
de catre clientul tcp si verific cu ajutorul lui daca clientul este nou 
sau doar s-a reconectat. Daca este un client nou, il adaug in vectorul 
de clienti, marcandu-l ca fiind conectat. In cazul in care clientul exista 
in vectorul de clienti, inseamna ca acesta a fost offline, iar acum il 
marchez ca si conectat si ii trimit mesajele despre topicurile la care e 
abonat(cele salvate in coada conform store&forwarding).

Mai avem cazul in care se primesc date de la client fara a se crea o 
conexiune noua. In acest caz, am doua variante. Daca nu se primesc date, 
inchid conexiunea, iar clientul va fi offline. Daca se primesc date, le salvez
intr-o structura de mesaj. Extrag topicul si valoarea sf si creez o noua 
subscriptie. In functie de valoarea comenzii, voi abona sau dezabona clientul 
la acel topic. Pentru abonare, verific daca clientul este deja abonat la acel
topic si in functie de asta, updatez doar sf-ul sau adaug noua subscriptie.
Cand dezabonez clientul, verific daca clientul chiar este abonat.


TRATARE ERORI

Verific intotdeauna daca se realizeaza trimiterea si primirea datelor, 
daca formatul mesajelor(dimensiunea, comenzi, nr parametri) este corect 
si in caz contrar, inchei executia programului si afisez eroarea.

TESTARE SI PROBLEME INTAMPINATE

Am dezactivat algoritmul lui Neagle.

Am testat tema folosind comanda sugerata pe forum:
    'python3 udp_client.py --source-port 1234 --input_file three_topics_payloads.json --mode random --delay 2000 127.0.0.1 8080'

Mesajele se trimit corect, insa atunci cand deconectez un client am eroarea 
'stack smashing detected. Aborted(core dumped)'. Am verificat cum lucrez cu memoria si 
nu am reusit sa gasesc greseala. Cu toate acestea, aplicatia continua sa functioneze 
si dupa primirea erorii(la conectarea altor clienti).

Am intampinat si o problema semnalata pe forum, legata de stringul "Ana are mere", 
care nu are terminator de sir.
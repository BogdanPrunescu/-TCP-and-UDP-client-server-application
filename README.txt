Prunescu Bogdan-Andrei 321CA

Aplcatie client-server TCP si UDP pentru gestionarea mesajelor

	Serverul si clientii sunt facuti pe baza laboratoarelor 7 si 5, de unde
m-am inspirat pentru functionalitatiile de baza (pornirea serverului si a
clientului si trimiterea/receptionarea mesajelor). Serverul se foloseste de
doua socketuri pentru a comunica cu clientii: unul este de tip TCP, iar
celalalt de tip UDP. Cele doua socketuri sunt legate la acelasi IP si port
	Pentru implementarea protocolului care este folosit de clienti si de
server am folosit structua homework_prot din fisierul common.h. Field-ul
"command" reprezinta de fel de comanda doreste sa execute expeditorul. Restul
field-urilor sunt folosite in functie de ce comanda se executa. Pe langa
comenzile de baza ("subscribe", "unsubscribe", "exit") am mai implementat inca
doua comenzi pe care serverul le vrea sa le foloseasca:
	"news" -> clientul va stii ca a primit de la server un mesaj de la
un topic la care este abonat. Acesta va afisa mesajul primit in fieldul
udp_packet.
	"refuse" -> serverul trimite comanda refuse catre client doar atunci
cand exista deja un client conectat cu acelasi id. Clientul care vrea sa se
conecteze isi va termina executa si se va inchide.

	In plus, am implementat si inchiderea clientilor daca serverul este
inchis, tratarea comenzilor invalide.

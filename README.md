
# TCP server

#### Anotace

Cílem úlohy je vytvořit vícevláknový server pro TCP/IP komunikaci a implementovat komunikační protokol podle dané specifikace.

#### Zadání

Vytvořte server pro automatické řízení vzdálených robotů. Roboti se sami přihlašují k serveru a ten je navádí do cílové lokace v souřadnicové síti. Pro účely testování každý robot startuje na náhodných souřadnicích a jeho cíl se nachází vždy na souřadnici (0,0). Cesta končí vyzvednutím tajného vzkazu, který se nachází právě na cílové souřadnici. Server zvládne komunikovat z více roboty najednou a implementuje bezchybně komunikační protokol.

#### Detailní specifikace

Komunikace mezi serverem a roboty je realizována plně textovým protokolem. Každý příkaz je zakončen dvojicí speciálních symbolů „\r\n“. Server musí dodržet komunikační protokol do detailu přesně, ale musí počítat s nedokonalými firmwary robotů (viz sekce Speciální situace). Maximální délka zprávy od robota je 100 znaků.

#### Zprávy serveru:

Název	Zpráva	Popis
* SERVER_USER 100 LOGIN\r\n	Výzva k zadání uživatelského jména
* SERVER_PASSWORD	101 PASSWORD\r\n	Výzva k zadání uživatelského hesla
* SERVER_MOVE	102 MOVE\r\n	Příkaz pro pohyb o jedno pole vpřed
* SERVER_TURN_LEFT	103 TURN LEFT\r\n	Příkaz pro otočení doleva
* SERVER_TURN_RIGHT	104 TURN RIGHT\r\n	Příkaz pro otočení doprava
* SERVER_PICK_UP	105 GET MESSAGE\r\n	Příkaz pro vyzvednutí zprávy
* SERVER_OK	200 OK\r\n	Kladné potvrzení
* SERVER_LOGIN_FAILED	300 LOGIN FAILED\r\n	Chybné heslo
* SERVER_SYNTAX_ERROR	301 SYNTAX ERROR\r\n	Chybná syntaxe zprávy
* SERVER_LOGIC_ERROR	302 LOGIC ERROR\r\n	Zpráva odeslaná ve špatné situaci

Zprávy klienta:

* Název	Zpráva	Popis	Ukázka	Maximální délka
* CLIENT_USER	<user name>\r\n	Zpráva s uživatelským jménem. Jméno může být libovolná sekvence znaků kromě kromě dvojice \r\n.	Umpa_Lumpa\r\n	100
* CLIENT_PASSWORD	<password>\r\n	Zpráva s uživatelským heslem. Ověření hesla je popsáno v sekci autentizace. Zpráva může obsahovat maximálně 5 čísel a ukončovací sekvenci \r\n.	1009\r\n	7
* CLIENT_CONFIRM	OK <x> <y>\r\n	Potvrzení o provedení pohybu, kde x a y jsou souřadnice robota po provedení pohybového příkazu.	OK -3 -1\r\n	12
* CLIENT_RECHARGING	RECHARGING\r\n	Robot se začal dobíjet a přestal reagovat na zprávy.		12
* CLIENT_FULL_POWER	FULL POWER\r\n	Robot doplnil energii a opět příjímá příkazy.		12
* CLIENT_MESSAGE	<text>\r\n	Text vyzvednutého tajného vzkazu. Může obsahovat jakékoliv znaky kromě ukončovací sekvence \r\n.	Haf!\r\n	100
Časové konstanty:

* Název	Hodota [s]	Popis
* TIMEOUT	1	Server i klient očekávají od protistrany odpověď po dobu tohoto intervalu.
* TIMEOUT_RECHARGING	5	Časový interval, během kterého musí robot dokončit dobíjení.
Komunikaci s roboty lze rozdělit do několika fází:

#### Autentizace

Každý robot je po přihlášení vyzván serverem k odeslání uživatelského jména (SERVER_USER) a posléze i hesla. Uživatelské jméno múže být libovolná sekvence znaků. Heslo je textová reprezentace součtu ASCII kódů všech znaků užívatelského jména. Server zkontroluje správnost zadaných údajů až poté, co obdrží uživatelské jméno i heslo. Pokud heslo není správně, informuje o tom robota a ukončí komunikaci.


Server dopředu nezná uživatelská jména a hesla robotů. Roboti proto mohou zvolit jakékoliv jméno, ale musí k němu vytvořit správné heslo. Heslem je dekadické číslo v textové reprezentaci. Číslo je součtem všech ASCII hodnot znaků obsažených v uživatelském jménu (bez ukončovací sekvence znaků). Zde je ukázka výpočtu hesla:

Uživatelské jméno: Haf

ASCII reprezentace: 72 97 102

Výsledné heslo je text: 271
Pohyb robota k cíli

Robot se může pohybovat pouze rovně a je schopen provést otočení na místě doprava i doleva. Po každém příkazu k pohybu odešle potvrzení, jehož součástí je i aktuální souřadnice. Pozor - roboti jsou v provozu již dlouhou dobu, takže začínají chybovat. Občas se stane, že se nepohnou kupředu. Tuto situaci je třeba detekovat a správně na ni zareagovat! Pozice robota není serveru na začátku komunikace známa. Server musí zjistit polohu robota (pozici a směr) pouze z jeho odpovědí. Z důvodů prevence proti nekonečnému bloudění robota v prostoru, má každý robot omezený počet pohybů (posunutí vpřed i otočení). Počet pohybů by měl být dostatečný pro rozumný přesun robota k cíli. Následuje ukázka komunkace. Server nejdříve pohne dvakrát robotem kupředu, aby detekoval jeho aktuální stav a po té jej navádí směrem k cílovým souřadnicím.

Tuto částo komunikace nelze přeskočit, robot očekává alespoň jeden pohybový příkaz - SERVER_MOVE, SERVER_TURN_LEFT nebo SERVER_TURN_RIGHT!

Pozor! Roboti občas chybují a nedaří se jim vykonat pohyb vpřed. V případě, že se nepohnou z místa, je nutné to detekovat a poslat příkaz k pohybu ještě jednou. Při rotaci roboti nechybují.

#### Vyzvednutí tajného vzkazu

Poté, co robot dosáhne cílové souřadnice (0,0), požádá server robota o vyzvednutí vzkazu. Pokud je robot požádán o vyzvednutí vzkazu a nenachází se na cílové souřadnici, spustí se autodestrukce robota a komunikace se serverem je přerušena. V opačném případě pošle serveru text vyzvednutého tajného vzkazu a server potvrdí příjem. Poté klient i server ukončí spojení.

Každý z robotů má omezený zdroj energie. Pokud mu začne docházet baterie, oznámí to serveru a poté se začne sám ze solárního panelu dobíjet. Během dobíjení nereaguje na žádné zprávy. Až skončí, informuje server a pokračuje v činnosti tam, kde přestal před dobíjením. Pokud robot neukončí dobíjení do časového intervalu TIMEOUT_RECHARGING, server ukončí spojení.


#### Chybové situace

Někteří roboti mohou mít poškozený firmware a tak mohou komunikovat špatně. Server by měl toto nevhodné chování detekovat a správně zareagovat.

#### Chyba při autentizaci

Server reaguje na chybnou autentizaci zprávou SERVER_LOGIN_FAILED. Tato zpráva je poslána pouze po té, co server přijme validní zprávu s uživatelským jménem + validní zprávu s uživatelským heslem a heslo neodpovídá uživatelskému jménu. (Validní == syntakticky korektní) V jiné situaci server zprávu SERVER_LOGIN_FAILED poslat nesmí.

#### Syntaktická chyba

Na syntaktickou chybu reagauje server vždy okamžitě po obdržení zprávy, ve které chybu detekoval. Server pošle robotovi zprávu SERVER_SYNTAX_ERROR a pak musí co nejdříve ukončit spojení. Syntakticky nekorektní zprávy:

Příchozí zpráva je delší než počet znaků definovaný pro každou zprávu (včetně ukončovacích znaků \r\n). Délky zpráv jsou definovány v tabulce s přehledem zpráv od klienta.
Příchozí zpráva syntakticky neodpovídá ani jedné ze zpráv CLIENT_CONFIRM, CLIENT_RECHARGING a CLIENT_FULL_POWER.
Každá příchozí zpráva je testována na maximální velikost a pouze zprávy CLIENT_PASSWORD, CLIENT_CONFIRM, CLIENT_RECHARGING a CLIENT_FULL_POWER jsou testovany na jejich obsah (zprávy CLIENT_USER a CLIENT_MESSAGE mohou obsahovat cokoliv).

#### Logická chyba

Logická chyba nastane pouze v jednom případě - když robot pošle info o dobíjení (CLIENT_RECHARGING) a po té pošle jakoukoliv jinou zprávu než CLIENT_FULL_POWER. Server na tuto chybu reaguje odesláním zprávy SERVER_LOGIC_ERROR a okamžitým ukončením spojení.

#### Timeout

Protokol pro komunikaci s roboty obsahuje dva typy timeoutu:

TIMEOUT - timeout pro komunikaci. Pokud robot nebo server neobdrží od své protistrany zprávu po dobu tohoto časového intervalu, považují spojení za ztracené a okamžitě ho ukončí.
TIMEOUT_RECHARGING - timeout pro dobíjení robota. Po té, co server přijme zprávu CLIENT_RECHARGING, musí robot nejpozději do tohoto časového intervalu odeslat zprávu CLIENT_FULL_POWER. Pokud to robot nestihne, server musí okamžitě ukončit spojení.
Speciální situace

Při komunikaci přes komplikovanější síťovou infrastrukturu může docházet ke dvěma situacím:

Zpráva může dorazit rozdělena na několik částí, které jsou ze socketu čteny postupně. (K tomu dochází kvůli segmentaci a případnému zdržení některých segmentů při cestě sítí.)
Zprávy odeslané brzy po sobě mohou dorazit téměř současně. Při jednom čtení ze socketu mohou být načteny obě najednou. (Tohle se stane, když server nestihne z bufferu načíst první zprávu dříve než dorazí zpráva druhá.)
Za použití přímého spojení mezi serverem a roboty v kombinaci s výkonným hardwarem nemůže k těmto situacím dojít přirozeně, takže jsou testovačem vytvářeny uměle. V některých testech jsou obě situace kombinovány.

Každý správně implementovaný server by se měl umět s touto situací vyrovnat. Firmwary robotů s tímto faktem počítají a dokonce ho rády zneužívají. Pokud se v protokolu vyskytuje situace, kdy mají zprávy od robota předem dané pořadí, jsou v tomto pořadí odeslány najednou. To umožňuje sondám snížit jejich spotřebu a zjednodušuje to implementaci protokolu (z jejich pohledu).

#### Optimalizace serveru

Server optimalizuje protokol tak, že nečeká na dokončení zprávy, která je očividně špatná. Například na výzvu k autorizaci pošle robot pouze část zprávy s uživatelským jménem. Server obdrží např. 120 znaků uživatelského jména, ale stále neobdržel ukončovací sekvenci \r\n. Vzhledem k tomu, že maximální délka zprávy je 100 znaků, je jasné, že přijímaná zpráva nemůže být validní. Server tedy zareaguje tak, že nečeká na zbytek zprávy, ale pošle zprávu SERVER_SYNTAX_ERROR a ukončí spojení. V principu by měl postupovat stejně při vyzvedávání tajného vzkazu.

V případě části komunikace, ve které se robot naviguje k cílovým souřadnicím očekává tři možné zprávy: CLIENT_CONFIRM, CLIENT_RECHARGIN nebo CLIENT_FULL_POWER. Pokud server načte část neúplné zprávy a tato část je delší než maximální délka těchto zpráv, pošle SERVER_SYNTAX_ERROR a ukončí spojení. Pro pomoc při optimalizaci je u každé zprávy v tabulce uvedena její maximální velikost.


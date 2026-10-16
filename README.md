# FS_PWR_Recruitment_Task

Rozwiązania zadań rekrutacyjnych do podprojektu Autonomy (Formula Student).

## Spis treści

1. [Struktura repozytorium](#struktura-repozytorium)
2. [Uruchamianie](#uruchamianie)
3. [Zadanie 1 – opis rozwiązania](#zadanie-1--opis-rozwiązania)
4. [Zadanie 1 – testy i dowód poprawności](#zadanie-1--testy-i-dowód-poprawności)
5. [Zadanie 1 – znane ograniczenia](#zadanie-1--znane-ograniczenia)
6. [Zadanie 2.1 – analiza czujników](#zadanie-21--analiza-czujników)
7. [Zadanie 2.2 – estymacja pozycji](#zadanie-22--estymacja-pozycji)
8. [Zadanie 2.2 – testy i wizualizacja](#zadanie-22--testy-i-wizualizacja)

---

## Struktura repozytorium

```
.
├── README.md
├── zadanie1.cpp            # 1.1 + 1.2
├── zadanie2.ipynb          # 2.1 – analiza i wykresy
├── zadanie2_2.cpp          # 2.2
├── img/                    # wykresy
└── tests/
    └── data/*.txt          # pliki wejściowe z przypadkami brzegowymi
```

---

## Uruchamianie

Wszystkie polecenia wykonuj z głównego katalogu repozytorium.

### Zadanie 1 (C++)

Potrzebny jest kompilator C++.

```bash
g++ zadanie1.cpp -o zadanie1
./zadanie1 tests/data/przyklad.txt
```

Program przyjmuje **jeden argument** – ścieżkę do pliku wejściowego:

```
<n>
<m>
<id> <x> <y>      # n takich linii
```

Wynik trafia na standardowe wyjście. Przy błędzie (brak argumentu, brak pliku, `n < 1`, `m < 0`) program kończy się kodem `1`.

### Testy zadania 1

```bash
g++ zadanie1.cpp -o zadanie1
python3 tests/check.py
```


### Zadanie 2.1 (Python)

```bash
pip install matplotlib numpy pandas jupyter
jupyter notebook zadanie2.ipynb
```

Notebook wczytuje `sensor1.csv` i `sensor2.csv` przez `pd.read_csv(...)` – jeśli pliki leżą w innym miejscu niż w ścieżce w pierwszej komórce, wystarczy ją zmienić.

### Zadanie 2.2 (C++)

```bash
g++ zadanie2_2.cpp -o zadanie2_2
./zadanie2_2 sensor1.csv sensor2.csv
```

Program po starcie czeka na liczbę `T` z zakresu `0.000 – 33.000` wpisywaną w terminalu
(po jednej w linii) i wypisuje pozycję auta w formacie `x y`. Zakończenie: `Ctrl+D`.

### Zadanie 2.2 Testy (python)
```bash
pip install matplotlib numpy pandas
python3 visualize.py
```
Skrypt automatycznie kompiluje program C++, przesyła do niego gęstą siatkę czasową (krok co 20 ms / 0.02 s) i generuje wykres porównawczy.

---

## Zadanie 1 – opis rozwiązania

Całość jest w jednym pliku `zadanie1.cpp`, każda funkcja ma komentarz opisujący jej działanie.

### 1.1 Deduplikacja pachołków

**Cel:** pachołki oddalone o `≤ m` są traktowane jako ten sam obiekt. Każda grupa blisko leżących pachołków jest scalana w nowy pachołek o współrzednych srednich pozycji usuwanych pachołków z nowym ID.

Rozwiązanie składa się z trzech kroków:

1. **Siatka przestrzenna** – `build_adjacency_list`.
   Zamiast porównywać każdą parę (`O(n²)`), dziele płaszczyznę na kwadratowe bloczki o boku `m` i zapisuje indeksy punktów w `std::map<pair<int,int>, vector<int>>`.
   Dzięki temu szukam powieleń pachołka tylko w tej samej komórce i 8 otaczających, dzięki temu nie musze sprawdzać za kadżdym razem wszystkich punktów. (Przy rownomiernie rozlozonych pachołkach daje to złożoność czasowa bliską `O(n)`). Wynikiem jest lista sąsiedztwa punktów położonych bliżej niż m.
2. **Składowe grafu (DFS)** – `dfs`.
   Grupy do scalenia to wierzchołki grafu. Dzięki temu scalanie jest **przechodnie**:
   jeśli A jest blisko B, a B blisko C, to A, B i C trafiają do jednej grupy, nawet gdy A i C są od siebie dalej niż `m`.
3. **Uśrednianie i nadawanie ID** – `deduplicate`.
   Dla grupy o więcej niż jednym elemencie liczymy średnią arytmetyczną `x` i `y`, nadajemy nowe ID (największe ID z wejścia + 1) i zapisuje listę usuniętych ID (posortowaną rosnąco).
   Pachołki bez sąsiadów przechodzą do nowej mapy bez zmian.

Wyjście: po jednej linii `CONE <nowe_id> <x> <y> [usunięte_id,…]` (3 miejsca po przecinku).
Nowe pachołki są numerowane w kolejności pierwszego wystąpienia grupy w pliku wejściowym.

### 1.2 Najkrótsza otoczka

**Cel:** najkrótsza zamknięta łamana ogradzająca wszystkie pachołki to obwód **otoczki wypukłej**. Otoczka liczona jest na mapie po deduplikacji.

Zaimplementowany jest algorytm **Quickhull** (dziel i zwyciężaj) – `print_cones_loop`
i `find_outliers_rec`:

1. Wybieramy punkt `A` o najmniejszym `x` i `B` o największym `x` – prosta `AB` dzieli zbiór
   na punkty po jednej i po drugiej stronie.
2. Dla każdej strony wywołujemy rekurencyjnie `find_outliers_rec(A, B, punkty)`: znajduje ona najdalszy punkt `C` od odcinka `AB` (za pomocą iloczyna wektorowego)
   `cross_prod_signed`, którego znak mówi, po której stronie prostej leży punkt.
3. Punkty wewnątrz trójkąta `ABC` nie mogą należeć do otoczki i są odrzucane. Zostają tylko punkty na zewnątrz krawędzi `AC` oraz `CB` - dla nich wchodze głębiej i szukam dla nich bardiej odstajacych punktów.
4. Rekurencja kończy się, gdy po zewnętrznej stronie krawędzi nie ma już żadnego punktu
   (`max_idx == -1`) – wtedy krawędź jest bokiem otoczki i dopisuje jej koniec do wyniku.

Obwód jest liczony jako suma kolejnych punktów (algorytm zwraca je w kolejności w jakiej przebiega otoczka) ostatni punkt łączy się z pierwszym.

Złożoność: średnio `O(n log n)`, w najgorszym przypadku `O(n²)`.

Wyjście: `ENCLOSURE <obwód> [id,…]` – kolejność ID jest kolejnością obejścia otoczki.

### Przypadki brzegowe i problemy

* **Precyzja zmiennoprzecinkowa.** Przez to ze działmy na typie double zmienne mają błedy zmiennoprzecinkowe, więc dodaje 1e-9 żeby przyrównujac wartości punktów nie dostać nieskończonej pętli lub nie odzucić punktów leżących na pograniczu.
* **Odległość równa m jest scalana**  zgodnie z treścią („minimalna dopuszczana odległość”).
* **m = 0.** Rozmiar komórki = 1.0 (żeby unie dzielić przez 0), scalane są punkty leżące dokładnie w tym samym miejscu.
* **Scalanie pachołków** Tworz spójny graf przed scaleniem, a nie przechodze po kolei, ponieważ przejscie po kolei dawałoby różne wyniki w zależności od tego od którego pachołka zaczniemy. (Dzięki grafowi wynik jest deterministyczny).
* **Unikalność ID.** Nowe ID startują od maxID + 1, gdzie maxID to największe ID z pliku (ID nie muszą być kolejne ani zaczynać się od 1).
* **Punkty współliniowe z krawędzią** są pomijane (porównania `> 0`), więc na liście są tylko wierzchołki otoczki.
* **Punkty wewnątrz otoczki** (np. pachołki 6 i 7 w przykładzie) są odrzucane już na pierwszych
  poziomach rekurencji, więc nie wpływają na dalsze obliczenia.

---

## Zadanie 1 – testy 

**1. Przypadki ręczne (`tests/data/`)**

| Plik | Co sprawdza | Wynik programu |
|---|---|---|
| `przyklad.txt` | przykład z treści zadania | zgodny z treścią zadania |
| `brak_duplikatow.txt` | kwadrat 10×10, nic nie jest scalane | brak `CONE`, obwód `40.000` |
| `odleglosc_rowna_m.txt` | 4 punkty o boku kwadratu dokładnie `m` – scalanie przy `d = m` | jedno `CONE 5 0.500 0.500 [1,2,3,4]` |
| `lancuch.txt` | odczyty co `0.9` (skrajne dalej niż `m`) – scalanie przechodnie + punkt daleko | `CONE 6 1.350 0.000 [1,2,3,4]` |
| `wspoliniowe_i_wnetrze.txt` | punkt współliniowy z przekątną i punkty wewnątrz | obwód `28.284`, tylko 4 wierzchołki |
| `ujemne_wspolrzedne.txt` | siatka z `floor` dla ujemnych współrzędnych | 2 poprawne `CONE` |
| `jeden_punkt.txt` | zdegenerowany zbiór: 1 pachołek | obwód `0.000` (zob. ograniczenia) |
| `wszystko_scalone.txt` | po scaleniu zostaje 1 punkt | obwód `0.000` (zob. ograniczenia) |

Dodatkowo ręczne testy błędnych danych (kod wyjścia 1).


---

## Zadanie 1 – znane ograniczenia

Poniższe rzeczy wyszły podczas testowania, ale żadna nie wpływa na przypadki z realnego toru
1. **Rekurencyjny DFS.** Łańcuch kilkuset tysięcy pachołków - każdy bliżej niż `m` od następnego przy bardzo dużej ilości pachołków może przepełnić stos (300.000+ pachołków). Tor raczej nie ma tylu pachołków, ale można użyć jawnego stosu w razie potrzeby działania na tak dużych liczbach.
2. **Walidacja wejścia** ogranicza się do `n ≥ 1` i `m ≥ 0` – program nie wypisuje komunikatów o błędach, tylko zwraca kod `1`.

---

## Zadanie 2.1 – analiza czujników

Cała analiza (kod + wykresy) jest w notebooku `zadanie2.ipynb`. Poniżej streszczenie
wniosków.

### Podsumowanie

| | **Sensor 1** | **Sensor 2** |
|---|---|---|
| Częstotliwość próbkowania | wysoka, krok ok. **0,025 s** (~40 Hz) | niska, krok ok. **0,5 s** (~2 Hz), ~20× rzadziej |
| Wygląd sygnału | gładki, ciągły | „kanciasty”, pojedyncze skoki |
| Charakter błędu | **dryf** – błąd systematyczny narastający w czasie | **szum i pojedyncze outliery**, brak narastającego dryfu |
| Znak osi `y` | – | **odwrócony** względem sensora 1 |
| Prędkość wyliczona z różnic | płynna, realistyczna dla bolidu | nierealne, duże skoki |
| Prawdopodobne źródło | odometria kół + kąt skrętu lub całkowanie IMU | odbiornik GPS |


### Podejrzewane źródła błędów

**Sensor 1 – dryf.** Sensor najprawdopodobniej opiera się na obrotach kół połączonych z kątem
skrętu albo na całkowaniu przyspieszeń z IMU. W obu przypadkach błąd się kumuluje, bo każda pozycja jest liczona z poprzedniej. Możliwe przyczyny w prawdziwym aucie:

* poślizgi opon,
* niedokładnie znany promień koła (ciśnienie, zużycie, temperatura opony),
* drgania, wyboje i nierówności nawierzchni zakłócające IMU,

**Sensor 2 – szum i outliery.** Zachowuje się jak odbiornik GPS: pozycja jest absolutna, ale jest rzadko próbkowana i ma losowy szum. Pojedyncze skoki mogą pochodzić od chwilowego pogorszenia odbioru satelitów czy zakłuceń sygnału. Odwrócony znak osi `y` to najpewniej różnica konwencji układu współrzędnych między czujnikami (np. układ lewo- i prawoskrętny), a nie fizyczny błąd pomiaru – wystarczy go skorygować.

**Wniosek ogólny:** czujniki mają rózne wady (ale dane z nich się uzupełniają) – sensor 1 jest gęsty i gładki, ale dryfuje, sensor 2 jest rzadki i zaszumiony, ale „trzyma” globalną pozycję.


## Zadanie 2.2 - estymacja pozycji
Aplikacja w zadanie2_2.cpp realizuje fuzję danych z obu sensorów w celu wyznaczenia ciągłej, rzeczywistej trajektorii pojazdu oraz estymacji pozycji (x,y) dla dowolnej chwili czasowej T od 0 do 33s

### Koncepcja i algorytm

1. **Korekta osi Y:** Podczas wczytywania `sensor2.csv` współrzędne y są mnożone przez -1, aby uzgodnić układy współrzędnych obu czujników.
2. **Model błędu (Dryf kursu (t)):** Zakładam, że Sensor 1 bardzo dokładnie mierzy odległości (długości wektorów przesunięć), ale jego głównym błędem jest powoli narastający błąd w czasie. Każdy mały krok z Sensora 1 jest obracany o kąt (t) za pomocą macierzy rotacji:
   $$x_{\text{new}} = x_{\text{prev}} + \Delta x \cos(\psi) + \Delta y \sin(\psi)$$
   $$y_{\text{new}} = y_{\text{prev}} - \Delta x \sin(\psi) + \Delta y \cos(\psi)$$
3. **Parametryzacja i interpolacja kątów (`ANGLE_STEP = 2.0`):** Kąt korekty $\psi$ jest wyznaczany w punktach węzłowych co 2 sekundy, a pomiędzy nimi interpolowany liniowo. Zapewnia to gładkość funkcji błędu i uniemożliwia powstanie szumu o wysokiej częstotliwości.
4. **Nieliniowa optymalizacja:**
   * Wyliczany jest numeryczny Jakobian (wpływ zmiany każdego kąta na pozycję).
   * Tworzony i rozwiązywany jest układ równań normalnych $(J^T J + \lambda I) \Delta = -g$ przy użyciu własnej implementacji **eliminacji Gaussa z częściowym wyborem elementu głównego** (`solveLinear`).
   * Funkcja kosztu uwzględnia błąd dopasowania do GPS (MSE), karę za poszarpanie wykresu kątów (`SMOOTHING = 0.09`) oraz sztywną kotwicę na starcie trasy (`START_WEIGHT = 20.0`, wymuszającą $\psi(0) = 0$).
5. **Iteracyjne odrzucanie outlierów:** Optymalizacja wykonywana jest w 3 rundach. Po każdej rundzie mierzona jest odległość dopasowanej trasy od punktów GPS. Punkty oddalone o więcej niż `1.0 m` (`OUTLIER_DIST`) są oznaczane jako szpilki/outliery (`use[i] = false`) i ignorowane w kolejnych przebiegach.
6. **Estymacja pozycji w chwili $T$:** Po wyznaczeniu ostatecznej, gładkiej ścieżki program odpowiada na zapytania użytkownika ze standardowego wejścia, wyznaczając pozycję $(x, y)$ za pomocą interpolacji liniowej (`interpolate`).


### Wybór algorytmu, dobór hiperparametrów i refleksje

- **Analiza problemu vs. algorytm:** Samodzielnie przeanalizowałem zachowanie obu czujników i zidentyfikowałem ich cechy (dryf kursu w Sensorze 1, zaszumienie i odwróconą oś Y w Sensorze 2) oraz zaproponowałem koncepcję fizycznego "odkręcania" błędu. Jednak przez to ze sam algorytm (Levenberg-Marquardt z wygładzaniem) ma charakter nieliniowy to nie wiedziałem jak go dokładnie rozwiązać i oprogramować, więc skorzystałem z internetu, i wzorowałem się na podobnych algorytmach dostosowując do konkretnego problemu jaki miałem rozwiązać
- **Ręczny dobór hiperparametrów:** Kluczowe parametry modelu (`ANGLE_STEP = 2.0`, `SMOOTHING = 0.09`, `OUTLIER_DIST = 1.0`, `START_WEIGHT = 20.0`) zostały dobrane iteracyjnie – poprzez modyfikowanie wartości i analizę wizualną oraz numeryczną uzyskiwanych trajektorii.
- **Liczba rund czyszczenia (3 pętle):** Liczba 3 rund filtrowania outlierów została ustalona eksperymentalnie. Po 3. przejściu zestaw punktów stabilizuje się i dalsze iteracje nie przynoszą już zauważalnej poprawy jakości wyznaczonej ścieżki.

---

## Zadanie 2.2 – testy i wizualizacja

Do weryfikacji poprawnego działania algorytmu powstał skrypt w Pythonie `visualize.py`.

### Przebieg testu:
1. Skrypt automatycznie kompiluje program `zadanie2_2.cpp`.
2. Generuje gęstą siatkę czasową z wąskim krokiem **20 ms** ($0.02\text{ s}$) od $0.000\text{ s}$ do $33.000\text{ s}$ (łącznie 1651 zapytań).
3. Przesyła punkty czasowe strumieniem `stdin` do skompilowanego programu C++ i odbiera odpowiadające im pozycje $(x, y)$.
4. Rysuje zbiorczy wykres porównawczy w `matplotlib` i zapisuje go do pliku `fuzja_sensorow_wykres.png`.

### Wynik wizualny testu:
* **Surowy Sensor 1 (czerwona przerywana linia):** Pokazuje płynny ruch, który jednak drastycznie odjeżdża od właściwego toru z powodu skumulowanego dryfu.
* **Surowy Sensor 2 (niebieskie punkty):** Pokazuje rzadkie, zaszumione pomiary GPS oraz wyraźne szpilki/outliery.
* **Estymowana trajektoria C++ (zielona ciągła linia):** Idealnie prostuje dryf Sensora 1, zachowuje pełną płynność ruchu przy gęstym próbkowaniu (20 ms) oraz całkowicie ignoruje odrzucone szpilki GPS.


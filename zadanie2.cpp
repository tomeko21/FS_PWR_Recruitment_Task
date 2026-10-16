#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <iomanip>

using namespace std;

struct Point {
    double t;
    double x;
    double y;
};

const double ANGLE_STEP = 2.0; // co ile sekund mamy jedna wartosc psi -miedzy nimi interpolacja (Jak długa jest kazda prosta krzywej w [s])
const double SMOOTHING = 0.09; // kara za ostre kąty krzywej psi
const double OUTLIER_DIST = 1.0; // GPS dalej niz 1 m od dopasowania = outlier
const double START_WEIGHT = 20.0; // jak mocno wymuszamy psi = 0 w chwili t = 0
const double MAX_TIME = 33.0;


// Wczytuje plik csv: timestamp,x,y (pierwsza linia to naglowek)
vector<Point> loadCSV(const string& filename, bool invertY) {
    vector<Point> data;
    ifstream file(filename);
    if (!file.is_open()) return data;

    string line;
    getline(file, line); // pomijamy naglowek

    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string ts, xs, ys;

        if (getline(ss, ts, ',') && getline(ss, xs, ',') && getline(ss, ys, ',')) {
            Point p;
            p.t = stod(ts);
            p.x = stod(xs);
            p.y = stod(ys);
            if (invertY) p.y = -p.y;
            data.push_back(p);
        }
    }
    return data;
}

// Liniowa interpolacja pozycji w chwili T
Point interpolate(const vector<Point>& data, double T) {
    //Jeśli T poza zakresem zwraca skrajny punkt
    if (T <= data.front().t) return data.front();
    if (T >= data.back().t) return data.back();


    // Szukam 1 punktu z t >= T
    int i = 1;
    while (data[i].t < T)
    {
        i++;  
    }

    // Obliczenie gdzie znajdywać się bedzie punkt w czasie T (funkcja liniowa międyz A i B)
    Point a = data[i - 1];
    Point b = data[i];
    double f = (T - a.t) / (b.t - a.t);

    Point result;
    result.t = T;
    result.x = a.x + f * (b.x - a.x);
    result.y = a.y + f * (b.y - a.y);

    return result;
}

// Blad kursu w chwili t: liniowa interpolacja miedzy kątami w wezlach
double headingErrorAt(const vector<double>& psi, double t) {

    // Numer kąta z lewej strony 
    double pos = t / ANGLE_STEP;
    int i = (int)pos;

    // Jeśli jestem na ostatnim kącie nie ma z prawej juz, wiec zwracam ostatni kąt
    if (i >= (int)psi.size() - 1) 
    {
        return psi.back();
    }


    double f = pos - i; // W ktorym miejscu między kątami jestem
    return psi[i] + f * (psi[i + 1] - psi[i]);
}

// Poprawiona trasa - obracam kroki sensora 1
vector<Point> buildPath(const vector<Point>& s1, const vector<double>& psi) {
    vector<Point> path;
    path.push_back(s1[0]); // Punkt startowy jest znany

    for (int k = 1; k < (int)s1.size(); k++) {
        // Ronica w połozeniu punktow na osiach
        double dx = s1[k].x - s1[k - 1].x;
        double dy = s1[k].y - s1[k - 1].y;


        double tMid = (s1[k].t + s1[k - 1].t) / 2.0; // Timestamp miedzy odczytami
        double a = headingErrorAt(psi, tMid); // Rotacja w danym momencie

        // Obracam krok o -a (cofam błąd kursu) i dodaję do ostatniego POPRAWIONEGO punktu.
        // Obrót nie zmienia długości kroku, tylko jego kierunek.
        Point p;
        p.t = s1[k].t;
        p.x = path.back().x + dx * cos(a) + dy * sin(a);
        p.y = path.back().y - dx * sin(a) + dy * cos(a);
        path.push_back(p);
    }
    return path;
}

// Roznice (trasa - GPS) w chwilach pomiarow GPS: najpierw x, potem y, dla kazdego punktu.
vector<double> computeResiduals(const vector<Point>& s1, const vector<Point>& gps, const vector<bool>& use, const vector<double>& psi) {

    vector<Point> path = buildPath(s1, psi); //Poprawiona ścierzka odczytow sensora 1
    vector<double> r;
    for (int i = 0; i < (int)gps.size(); i++) {
        if (use[i]) {
            // Jeśli punkt GPS nie jest outlierem licze rozbieznosc miedzy punktem GPS a punktami z sensora 1 po rotacji
            Point p = interpolate(path, gps[i].t);
            r.push_back(p.x - gps[i].x);
            r.push_back(p.y - gps[i].y);
        } else {
            // Jeśli punkt GPS to outlier bład = 0
            r.push_back(0.0);
            r.push_back(0.0);
        }
    }
    // Dodatkowa wartość kara za zmiane kąta pierwszego (wtedy sensor nie ma dryfu)
    r.push_back(START_WEIGHT * psi[0]);
    return r;
}

// Funkcja kosztu  (suma kwadratow bledow + kara za ostre kąty)
double computeCost(const vector<double>& r, const vector<double>& psi) {
    double c = 0;

    // Suma kwadratow bledow
    for (int i = 0; i < (int)r.size(); i++) 
    {
        c += r[i] * r[i];
    }
    // Suma kwadratow kar
    for (int j = 1; j + 1 < (int)psi.size(); j++) 
    {
        double d2 = psi[j - 1] - 2 * psi[j] + psi[j + 1]; // Roznica kątow 
        c += SMOOTHING * d2 * d2;
    }
    return c;
}

// Rozwiazuje uklad rownan A * x = b (eliminacja Gaussa)
vector<double> solveLinear(vector<vector<double> > A, vector<double> b) {
    int n = b.size();
    for (int col = 0; col < n; col++) 
    {
        //Szukamy wiersza z największą wartością w obecnej kolumnie eby nie dzielić przez liczby bliskie 0 (błędy zaokrągleń)
        int best = col;
        for (int row = col + 1; row < n; row++) 
        {
            if (fabs(A[row][col]) > fabs(A[best][col]))
            {
                best = row;
            }
        }
        swap(A[col], A[best]);
        swap(b[col], b[best]);

        // Eliminacja w przód – zerujemy liczby pod główną przekątną
        for (int row = col + 1; row < n; row++) 
        {
            double f = A[row][col] / A[col][col];
            for (int k = col; k < n; k++) 
            {
                A[row][k] -= f * A[col][k];
            }
            b[row] -= f * b[col];
        }
    }

    // Podstawianie wstecz – wyliczamy wartości x od ostatniej do pierwszej
    vector<double> x(n);
    for (int row = n - 1; row >= 0; row--) 
    {
        double s = b[row];
        for (int k = row + 1; k < n; k++)
        {
            s -= A[row][k] * x[k];
        } 
        x[row] = s / A[row][row];
    }
    return x;
}

// Dopasowanie krzywej psi (metoda Gaussa-Newtona z tlumieniem Levenberga-Marquardta)
vector<double> fitHeading(const vector<Point>& s1, const vector<Point>& gps, const vector<bool>& use, vector<double> psi) {
    int n = psi.size();
    double lambda = 1.0; // początkowa ostrożność kroku optymalizacji (wieksza - mnijesze kroki mniejsza - wieksze kroki)

    vector<double> r = computeResiduals(s1, gps, use, psi); //roznice GPS a sensor
    double cost = computeCost(r, psi);

    for (int iter = 0; iter < 100; iter++) {
        int m = r.size();

        // Jakobian (tabela nachyleń) jak mocno każdy błąd reaguje na ruszenie każdego kąta
        vector<vector<double> > J(m, vector<double>(n));
        double h = 1e-5; //mała zmiana kąta 
        for (int k = 0; k < n; k++) { 
            // sprawdzam co sie stanie jak zmienie k-ty kąt
            vector<double> psi2 = psi;
            psi2[k] += h;
            vector<double> r2 = computeResiduals(s1, gps, use, psi2);

            // Jak zmienił się błąd numer row (odczytu GPS) po ruszeniu k-tego kąta
            for (int row = 0; row < m; row++) 
            {
                J[row][k] = (r2[row] - r[row]) / h;
            }
        }


        // Szukam kroku delta (o ile ruszyć wszystkie kąty):
        // nowe błędy ≈ r + J * delta. Nadaje się układ równań A * delta = -g, gdzie:
        //   A[a][b] = jak bardzo kąty a i b wpływają na te same błędy
        //   g[a]    = w którą stronę rośnie błąd, gdy zwiększam kąt a
        vector<vector<double>> A(n, vector<double>(n, 0.0));
        vector<double> g(n, 0.0);
        for (int a = 0; a < n; a++) {
            for (int b = 0; b < n; b++) {
               // suma po błędach: wpływ kąta a razy wpływ kąta b = jak bardzo oba kąty ciągną te same punkty (sprzężenie)
                for (int row = 0; row < m; row++) 
                {
                    A[a][b] += J[row][a] * J[row][b];
                }
            }
            // sumuję po wszystkich błędach: wpływ kąta a razy aktualny błąd (im blizej 0 g[a] tym lepiej)
            for (int row = 0; row < m; row++)
            {
                g[a] += J[row][a] * r[row];
            }
        }

        // Kara za zgięcie wykresu kątów korekty, sprawdzam, jak bardzo wykres jest w środku zgięty
        for (int j = 1; j + 1 < n; j++) {
            int idx[3] = {j - 1, j, j + 1}; 
            double coef[3] = {1, -2, 1}; // czułość d2 na każdy z kątow
            double d2 = psi[j - 1] - 2 * psi[j] + psi[j + 1];

            for (int a = 0; a < 3; a++) 
            {
                // czułość × zgięcie, w którą stronę kara ciągnie ten kąt
                g[idx[a]] += SMOOTHING * coef[a] * d2;
                for (int b = 0; b < 3; b++)
                {
                    // czułość × czułość: jak mocno te trzy kąty są związane karą
                    A[idx[a]][idx[b]] += SMOOTHING * coef[a] * coef[b];
                }
            }
        }

        // Probujemy zrobić krok, jesli koszt nie spada, zwiekszamy lambde
        bool improved = false;
        double biggestStep = 0;
        for (int attempt = 0; attempt < 12 && !improved; attempt++) 
        {
            vector<vector<double> > A2 = A;
            vector<double> rhs(n); 
            for (int a = 0; a < n; a++) 
            {
                A2[a][a] += lambda; // Dodaje do maciezy lambde zeby krok był stabilny
                rhs[a] = -g[a]; // -g - prawa strona rownania (kierunek spadku gradientu)
            }

            //Wyznaczenie delty z układu A2 * delta = rhs
            vector<double> delta = solveLinear(A2, rhs);

            // Naniesienie poprawek kątow 
            vector<double> psiNew = psi;
            biggestStep = 0;
            for (int a = 0; a < n; a++) {
                psiNew[a] += delta[a];
                //Sprawdzenie największej poprawki
                if (fabs(delta[a]) > biggestStep) 
                {
                    biggestStep = fabs(delta[a]);
                }
            }

            // Obliczenie kosztu dal nowych kątow
            vector<double> rNew = computeResiduals(s1, gps, use, psiNew);
            double costNew = computeCost(rNew, psiNew);

            //Sprawdzenie czy zmiana kąta coś poprawiła
            if (costNew < cost) {
                psi = psiNew;
                r = rNew;
                cost = costNew;
                lambda /= 3.0;
                improved = true;
            } else {
                lambda *= 4.0;
            }
        }
        // Jeśli błąd nie zmniejsza się prawie wcale to przerywam szukanie
        if (!improved || biggestStep < 1e-7) break;
    }
    return psi;
}

int main(int argc, char* argv[]) {
    if (argc != 3 && argc != 4) {
        return 1;
    }

    // tryb debugowania do testow
    bool debug = (argc == 4);

    //Wczytanie danych z sensorow z pliku
    vector<Point> sensor1 = loadCSV(argv[1], false);
    vector<Point> sensor2 = loadCSV(argv[2], true); //odwrocona os Y

    // Sprawdzenie czy pliki zawierają dane
    if (sensor1.size() < 2 || sensor2.empty()) {
        return 1;
    }


    int numAngles = (int)ceil(sensor1.back().t / ANGLE_STEP) + 1; // Ile jest kątow do korekty
    vector<double> psi(numAngles, 0.0); // kąty korekty kursu lini w radianach
    vector<bool> use(sensor2.size(), true); // ktore odczyty GPS uzywamy (odzucamy outliery)

    // Pętla "ucząca" (zmniejsza bład i usuwa outliery)
    //Dopasuj -> odrzuc szum GPS -> dopasuj ponownie
    psi = fitHeading(sensor1, sensor2, use, psi);
    for (int round = 0; round < 3; round++) {
        vector<double> r = computeResiduals(sensor1, sensor2, vector<bool>(sensor2.size(), true), psi);
        // Usuwam szum z GPS (outliery)
        for (int i = 0; i < (int)sensor2.size(); i++) {
            double dist = sqrt(r[2 * i] * r[2 * i] + r[2 * i + 1] * r[2 * i + 1]);
            use[i] = (dist < OUTLIER_DIST);
        }
        psi = fitHeading(sensor1, sensor2, use, psi);
    }

    // Ostateczny przebieg
    vector<Point> path = buildPath(sensor1, psi);



    cout << fixed << setprecision(3);

    // Wczytywanie T od usera do przerwanai programu i wypisywanie wyniku
    double T;
    while (cin >> T) {
        if (T < 0.0 || T > MAX_TIME) continue;

        Point p = interpolate(path, T);
        cout << p.x << " " << p.y << '\n';
    }

    return 0;
}
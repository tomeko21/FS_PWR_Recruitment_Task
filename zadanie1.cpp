#include <iostream>
#include <stdio.h> 
#include <vector>
#include <cmath>
#include <fstream>
#include <map>
#include <iomanip>
#include <algorithm>
#include <climits>


using namespace std;

// Struktura pojedynczego pachołka z pliku
struct Point {
    long id;
    double x;
    double y;
};

// Struktura nowego pachołka ze scalenia 
struct NewPoint {
    long id;
    double x;
    double y;
    vector<long> rm_ids;
};

// Funkcja liczy kwadrat odległości między 2 punktami
double distance_squared(Point a, Point b)
{
    return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y);
}

// Funkcja liczy iloczyn wektorowy, zwraca odległość punktu P od prostej między punktami A i B 
// oraz po ktrej stronie prostej się znajduje punkt P (+ / -)
double cross_prod_signed(Point A, Point B, Point P)
{
    return (B.x - A.x) * (P.y - A.y) - (B.y - A.y) * (P.x - A.x);
}

// DFS - przeszukuje graf w głąb - znajduje pachołki które należą do tej samej grupy (leżą blisko siebie)
void dfs(int cur, const vector<vector<int>>& adj_list, vector<bool>& visited, vector<int>& to_merge)
{
    visited[cur] = true;
    to_merge.push_back(cur);

    for(size_t i = 0; i < adj_list[cur].size(); i++)
    {
        int neighbor = adj_list[cur][i];

        if(!visited[neighbor])
        {
            dfs(neighbor, adj_list, visited, to_merge);
        }
    }

}

// Funkcja odczytuje dane z pliku wejściowego
bool load_file(const string& filename, vector<Point>& points, int& n, double& m, long& maxID)
{
    ifstream file(filename);
    if (!file.is_open())
    {
        return false;
    }

    file >> n >> m;
    if (n < 1 || m < 0.0) return false;
    for(int i = 0; i < n; i++)
    {
        long id;
        double x, y;
        file >> id >> x >> y;
        points.push_back({id, x, y});

        // Sprawdzam największe id jakie jest, żeby wiedzieć od jakiego id zacząć numerować pachołki powstałe przez scalenie
        (id > maxID) && (maxID = id);
    }
    file.close();
    return true;
}

// Budowanie listy sąsiedztwa za pomocą siatki (sprawdzam tylko sąsiadujące kratki o bokach m)
vector<vector<int>> build_adjacency_list(const vector<Point>& points, double m)
{
    int n = points.size();
    map<pair<int, int>, vector<int>> grid;
    double cell_size = (m > 0) ? m : 1.0;
    vector<vector<int>> adj_list(n);
    double m_squared = m*m + 1e-9; //margines na błąd double

    // Przypisanie pachołkow do odpowiedniej kratki na siatce
    for (int i = 0; i < n; i++)
    {
        int cell_x = floor(points[i].x / cell_size);
        int cell_y = floor(points[i].y / cell_size);
        grid[{cell_x, cell_y}].push_back(i);
    }


    // Dla każdego punktu sprawdzam tylko sąsiadów z kratek wokół (3x3 wokół punktu)
    for(int i = 0; i < n; i++)
    {
        int cell_x = floor(points[i].x / cell_size);
        int cell_y = floor(points[i].y / cell_size);

        for(int cx = -1; cx <= 1; cx++)
        {
            for(int cy = -1; cy <= 1; cy++)
            {
                pair<int, int> neighbor_cell = {cell_x + cx, cell_y + cy};
                auto neighbors = grid.find(neighbor_cell);
                if(neighbors != grid.end())
                {
                    for(int j : neighbors->second)
                    {
                        // i < j zabezpiecza przed dodaniem krawedzi 2 razy
                        if(i < j && distance_squared(points[i], points[j]) <= m_squared)
                        {
                            adj_list[i].push_back(j);
                            adj_list[j].push_back(i);
                        }
                    }
                }
            }
        }
    }
    return adj_list;

}

//Funkcja scala pachołki które leżą zbyt blisko siebie
vector<NewPoint> deduplicate(long& maxID, const vector<vector<int>>& adj_list, const vector<Point>& points, vector<Point>& filtered_map)
{
    int n = points.size();
    long nextID = maxID + 1;
    vector<bool> visited(n, false);
    vector<NewPoint> new_points;
    for(int i = 0; i < n; i++)
    {
        if(!visited[i])
        {
            vector<int> to_merge;
            dfs(i, adj_list, visited, to_merge);

            // Jeśli gurpa ma wiecej niż 1 punkt scalam pachołki
            if(to_merge.size() > 1)
            {
                double sum_x = 0.0;
                double sum_y = 0.0;

                vector<long> ids_to_rm;
                for (int id : to_merge)
                {
                    sum_x += points[id].x;
                    sum_y += points[id].y;
                    ids_to_rm.push_back(points[id].id);
                }
                sort(ids_to_rm.begin(), ids_to_rm.end());

                // Obliczam wyśrodkowane współrzędne 
                double avg_x = sum_x / ids_to_rm.size();
                double avg_y = sum_y / ids_to_rm.size();

                NewPoint point;
                point.id = nextID++;
                point.x = avg_x;
                point.y = avg_y;
                point.rm_ids = ids_to_rm;
                
                new_points.push_back(point);

                filtered_map.push_back({point.id, point.x, point.y});
            }else{
                // Jeślio pachołek nie musiał być scalony przechodzi do nowej mapy
                filtered_map.push_back(points[i]);
            }
        }
    }
    return new_points;
}

// Funkcja wypisuje informacje o scalonych pachołkach
void print_new_cones(const vector<NewPoint>& new_points)
{
    cout << setprecision(3) << fixed;
    for (const auto& cone : new_points)
    {
        cout << "CONE " << cone.id << " " << cone.x << " " << cone.y << " [";
        for(size_t i = 0; i < cone.rm_ids.size(); i++)
        {
            cout << cone.rm_ids[i];
            if(i != cone.rm_ids.size() - 1)
                cout << ",";
        }
        cout << "]" << '\n';
    }
}

// rekurencyjna funkcja ktra buduje otoczke znajdując najdalsze punkty od lini międyz 2 punktami ktre są w jakiejś płaszczyznie najdalej wysunięte
void find_outliers_rec(Point A, Point B, const vector<Point>& points, vector<Point>& cone_loop)
{
    int max_idx = -1;
    double max_distance = 1e-9;

    // Wyszukiwanie punktu najdalszego od prostej AB
    for(size_t i = 0; i < points.size(); i++)
    {
        double distance = cross_prod_signed(A, B, points[i]);
        if(distance > max_distance)
        {
            max_idx = i;
            max_distance = distance;
        }
    }

    //Jeśli nie znajdzie się dalej wysunięty punkt to znaczy ze AB jest linia otoczki
    if(max_idx == -1)
    {
        cone_loop.push_back(B);
        return;
    }

    Point C = points[max_idx];

    // Znajduje punkty które leżą poza prostymi AC i CB
    vector<Point> outliers_AC, outliers_CB;
    for(size_t i = 0; i < points.size(); i++)
    {
        if (cross_prod_signed(A, C, points[i]) > 0)
        {
            outliers_AC.push_back(points[i]);
        }
        if (cross_prod_signed(C, B, points[i]) > 0)
        {
            outliers_CB.push_back(points[i]);
        }
    }

    // Sprawdzam dla nowo powstałej prostych czy istnieje punkt wystający za nie
    find_outliers_rec(A, C, outliers_AC, cone_loop);
    find_outliers_rec(C, B, outliers_CB, cone_loop);
}

// Główna funkcja ktora wyznacza najkrutsza otoczke 
void print_cones_loop(const vector<Point>filtered_map)
{
    if (filtered_map.empty()) return;

    if (filtered_map.size() == 1)
    {
        cout << "ENCLOSURE 0.000 [" << filtered_map[0].id << "]" << '\n';
        return;
    }

    //Znajduje najbardziej wysunięte punkty na współrzednej X (na + i -)
    int min_x_idx = 0;
    int max_x_idx = 0;
    for(size_t i = 1; i < filtered_map.size(); i++)
    {
        // Szukam min X, jak remis biore min Y
        if (filtered_map[i].x < filtered_map[min_x_idx].x || 
            (filtered_map[i].x == filtered_map[min_x_idx].x && filtered_map[i].y < filtered_map[min_x_idx].y))
        {
            min_x_idx = i;
        }

        // Szukam max X, jak remis biore max Y
        if (filtered_map[i].x > filtered_map[max_x_idx].x || 
            (filtered_map[i].x == filtered_map[max_x_idx].x && filtered_map[i].y > filtered_map[max_x_idx].y))
        {
            max_x_idx = i;
        }
    }

    Point A = filtered_map[min_x_idx];
    Point B = filtered_map[max_x_idx];

    // Dziele punkty na te pod i nad prostą AB
    vector<Point> over, under;
    for(size_t i = 0; i < filtered_map.size(); i++)
    {
        double val = cross_prod_signed(A, B, filtered_map[i]);
        if (val > 0)
        {
            over.push_back(filtered_map[i]);
        }
        else if(val < 0)
        {
            under.push_back(filtered_map[i]);
        }
    }


    vector<Point> cone_loop;
    cone_loop.push_back(A);

    //Rekurencyjna budowa otoczki
    find_outliers_rec(A, B, over, cone_loop);
    find_outliers_rec(B, A, under, cone_loop);

    if(cone_loop.size() > 1 && cone_loop[0].id == cone_loop.back().id)
    {
        cone_loop.pop_back();
    }


    // Obliczanie łączenej długości otoczki
    double distance = 0.0;
    for(size_t i = 0; i < cone_loop.size(); i++)
    {
        Point p1 = cone_loop[i];
        Point p2 = cone_loop[(i + 1) % cone_loop.size()];
        distance += sqrt(distance_squared(p1, p2));
    }

    // Wypisanie wyniku
    cout << "ENCLOSURE " << distance << " [";
    for(size_t i = 0; i < cone_loop.size(); i++)
    {
        cout << cone_loop[i].id << ((i + 1 < cone_loop.size()) ? "," : "");
    }
    cout << "]" << '\n';

}



int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        return 1;
    }

    int n;
    double m;
    long maxID = LONG_MIN;
    vector<Point> points;
    vector<Point> filtered_map;


    if (!load_file(argv[1], points, n, m, maxID))
    {
        return 1;
    }

    // Zad 1.1
    vector<vector<int>> adj_list = build_adjacency_list(points, m);
    vector<NewPoint> new_points = deduplicate(maxID, adj_list, points, filtered_map);
    print_new_cones(new_points);


    // Zad 1.2
    print_cones_loop(filtered_map);
}
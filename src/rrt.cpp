/**
* @file rrt.cpp
* @author vss2sn
* @brief Contains the RRT class
*/

#include "rrt.h"

/**
* @brief Find the nearest node that has been seen by the algorithm. This does not consider cost to reach the node.
* @param new_node Node to which the nearest node must be found
* @param n number of rows/columns
* @return Nearest node
*/
Node RRT::FindNearestPoint(Node& new_node, int n){
  Node nearest_node(-1,-1,-1,-1,-1,-1);
  std::vector<Node>::iterator it_v;
  std::vector<Node>::iterator it_v_store;
  //use just distance not total cost
  double dist = (double)(n*n);
  double new_dist = dist;
  for(it_v = point_list_.begin(); it_v != point_list_.end(); ++it_v){
    new_dist = (double)sqrt((double)(it_v->x_-new_node.x_)*(double)(it_v->x_-new_node.x_)+
                            (double)(it_v->y_-new_node.y_)*(double)(it_v->y_-new_node.y_));
    if(new_dist > threshold_) continue;
    if(CheckObstacle(*it_v, new_node)) continue;
    if(it_v->id_==new_node.id_) continue;
    if(it_v->pid_==new_node.id_) continue;
    if(new_dist >= dist) continue;
    dist = new_dist;
    it_v_store = it_v;
  }
  if(dist!=n*n){
    nearest_node = *it_v_store;
    new_node.pid_ = nearest_node.id_;
    new_node.cost_ = nearest_node.cost_ + dist;
  }
  return nearest_node;
}
/**
* @brief Check if there is any obstacle between the 2 nodes. As this planner is for grid maps, the obstacles are square.
* @param n_1 Node 1
* @param n_2 Node 2
* @return bool value of whether obstacle exists between nodes
*/
bool RRT::CheckObstacle(Node& n_1, Node& n_2){
  if (n_2.y_ - n_1.y_ == 0){
    double c = n_2.y_;
    for(auto it_v = obstacle_list_.begin(); it_v!=obstacle_list_.end(); ++it_v){
      if(!(((n_1.x_>=it_v->x_) && (it_v->x_>= n_2.x_)) || ((n_1.x_<=it_v->x_) && (it_v->x_<= n_2.x_)))) continue;
      if ((double)it_v->y_ == c) return true;
    }
  }
  else {
    double slope = (double)(n_2.x_ - n_1.x_)/(double)(n_2.y_ - n_1.y_);
    std::vector<Node>::iterator it_v;
    double c = (double)n_2.x_ - slope * (double)n_2.y_;
    for(auto it_v = obstacle_list_.begin(); it_v!=obstacle_list_.end(); ++it_v){
      if(!(((n_1.y_>=it_v->y_) && (it_v->y_>= n_2.y_)) || ((n_1.y_<=it_v->y_) && (it_v->y_<= n_2.y_)))) continue;
      if(!(((n_1.x_>=it_v->x_) && (it_v->x_>= n_2.x_)) || ((n_1.x_<=it_v->x_) && (it_v->x_<= n_2.x_)))) continue;
      double arr[4];
      // Using properties of a point and a line here.
      // If the obtacle lies on one side of a line, substituting its edge points
      // (all obstacles are grid sqaures in this example) into the equation of
      // the line passing through the coordinated of the two nodes under
      // consideration will lead to all four resulting values having the same
      // sign. Hence if their sum of the value/abs(value) is 4 the obstacle is
      // not in the way. If a single point is touched ie the substitution leads
      // ot a value under 10^-7, it is set to 0. Hence the obstacle has
      // 1 point on side 1, 3 points on side 2, the sum is 2 (-1+3)
      // 2 point on side 1, 2 points on side 2, the sum is 0 (-2+2)
      // 0 point on side 1, 3 points on side 2, (1 point on the line, ie,
      // grazes the obstacle) the sum is 3 (0+3)
      // Hence the condition < 3
      arr[0] = (double)it_v->x_+0.5 - slope*((double)it_v->y_+0.5) - c;
      arr[1] = (double)it_v->x_+0.5 - slope*((double)it_v->y_-0.5) - c;
      arr[2] = (double)it_v->x_-0.5 - slope*((double)it_v->y_+0.5) - c;
      arr[3] = (double)it_v->x_-0.5 - slope*((double)it_v->y_-0.5) - c;
      double count = 0;
      for (int i=0;i<4;i++){
        if(fabs(arr[i]) <= 0.000001) arr[i] = 0;
        else count +=arr[i]/fabs(arr[i]);
      }
      if(abs(count) < 3) return true;
    }
  }
  return false;
}
/**
* @brief Generates a random node
* @param n Number of rows/columns
* @return Generated node
*/
Node RRT::GenerateRandomNode(int n){
  std::random_device rd; // obtain a random number from hardware
  std::mt19937 eng(rd()); // seed the generator
  std::uniform_int_distribution<int> distr(0,n-1); // define the range
  int x = distr(eng);
  int y = distr(eng);
  Node new_node(x, y, 0, 0, n*x+y, 0);
  return new_node;
}

/**
* @brief Main algorithm of RRT
* @param grid Main grid
* @param n number of rows/columns
* @param start_in starting node
* @param goal_in goal node
* @param max_iter_x_factor Maximum number of allowable iterations before returning no path
* @param threshold_in Maximum distance per move
* @return path vector of nodes
*/
std::vector<Node> RRT::rrt(void *grid, int n, Node start_in, Node goal_in, int max_iter_x_factor, double threshold_in){
  start_ = start_in;
  goal_ = goal_in;
  threshold_ = threshold_in;
  int max_iter = max_iter_x_factor * n * n;
  int (*p_grid)[n][n] = (int (*)[n][n]) grid;
  CreateObstacleList(*p_grid, n);
  point_list_.push_back(start_);
  Node new_node = start_;
  (*p_grid)[start_.x_][start_.y_]=2;
  if (CheckGoalVisible(new_node)) return this->point_list_;
  int iter = 0;
  while(iter <= max_iter){
    iter++;
    new_node = GenerateRandomNode(n);
    if ((*p_grid)[new_node.x_][new_node.y_]!=0) continue;
    Node nearest_node = FindNearestPoint(new_node, n);
    if(nearest_node.id_ == -1) continue;
    (*p_grid)[new_node.x_][new_node.y_]=2;
    point_list_.push_back(new_node);
    if (CheckGoalVisible(new_node)) return this->point_list_;
  }
  Node no_path_node(-1,-1,-1,-1,-1,-1);
  point_list_.clear();
  point_list_.push_back(no_path_node);
  return point_list_;
}

/**
* @brief Check if goal is reachable from current node
* @param new_node Current node
* @return bool value of whether goal is reachable from current node
*/
bool RRT::CheckGoalVisible(Node new_node){
  if(!CheckObstacle(new_node, goal_)){
    double new_dist = (double)sqrt((double)(goal_.x_-new_node.x_)*(double)(goal_.x_-new_node.x_) + (double)(goal_.y_-new_node.y_)*(double)(goal_.y_-new_node.y_));
    if(new_dist <= threshold_){
      goal_.pid_ = new_node.id_;
      goal_.cost_ = new_dist + new_node.cost_;
      point_list_.push_back(goal_);
      return true;
    }
  }
  return false;
}

/**
* @brief Create the obstacle list from the input grid
* @param grid input grid for algorithm
* @param n Number of rows/columns
* @return void
*/
void RRT::CreateObstacleList(void *grid, int n){
  int (*p_grid)[n][n] = (int (*)[n][n]) grid;
  for(int i=0; i < n; i++){
    for(int j=0;j < n; j++){
      if((*p_grid)[i][j]==1){
        Node obs(i,j,0,0,i*n+j,0);
        obstacle_list_.push_back(obs);
      }
    }
  }
}

#ifdef BUILD_INDIVIDUAL
/**
* @brief Script main function. Generates start and end nodes as well as grid, then creates the algorithm object and calls the main algorithm function.
* @return 0
*/
int main(){
  int n = 8;
  int num_points = n*n;
  int grid[n][n];
  MakeGrid(grid, n);
  //Make sure start and goal are not obstacles and their ids are correctly assigned.
  Node start(0,0,0,0,0,0);
  start.id_ = start.x_ * n + start.y_;
  start.pid_ = start.id_;
  Node goal(n-1,n-1,0,0,0,0);
  goal.id_ = goal.x_ * n + goal.y_;
  grid[start.x_][start.y_] = 0;
  grid[goal.x_][goal.y_] = 0;
  PrintGrid(grid, n);

  RRT new_rrt;
  double threshold = 2;
  int max_iter_x_factor = 20;
  std::vector<Node> path_vector = new_rrt.rrt(grid, n, start, goal, max_iter_x_factor, threshold);
  PrintPath(path_vector, start, goal, grid, n);

  return 0;
}
#endif

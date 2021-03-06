#include <math.h>
#include "define.h"
#include "AOIManager.h"

namespace AOI{

void AOIManager::Enter(const Entity& entity){
    int gid = GetGid(entity.x(), entity.y());
    Enter(entity.id(), gid);
    EnterBroadcast(entity, entity.x(), entity.y());
}

void AOIManager::Enter(uint64_t id, int gid){
    /*
    auto it = grids_.find(gid);
    if (it != grids_.end()){
        it->second.insert(id);
        printf("Enter: gid:%d->entityid:%lu\n",gid, id);
    }*/
    auto it = map_.find(gid);
    if (it != map_.end()){
        it->second->Enter(id, ENTITY_TYPE_PC);
    }
}

void AOIManager::Leave(const Entity& entity){
    int gid = GetGid(entity.x(), entity.y());
    Leave(entity.id(), gid);
    LeaveBroadcast(entity, entity.x(), entity.y());
}

void AOIManager::Leave(uint64_t id, int gid){
    /*
    auto it = grids_.find(gid);
    if (it != grids_.end()){
        it->second.erase(id);
        printf("Leave: gid:%d->entityid:%lu\n",gid, id);
    }
    */
   auto it = map_.find(gid);
   if (it != map_.end()){
       it->second->Leave(id, ENTITY_TYPE_PC);
   }
}

/*
* entity存放的坐标是旧的坐标
*/
void AOIManager::Move(const Entity& entity, int x, int y){
    int old_gid = GetGid(entity.x(), entity.y());
    int new_gid = GetGid(x, y);
    if (old_gid == new_gid){
        MoveBroadcast(entity, x, y);
        return;
    }else{
        Leave(entity.id(), old_gid);
        Enter(entity.id(), new_gid);
        move_cross_grid(entity, x, y);
    }
}

void AOIManager::InitGrids(){
    /*
    int width = width_;
	int height = height_;
	//将场景中的每个坐标都转换为9宫格坐标
	for (int w = 0; w <= width; ++w)
	{
		for (int h = 0; h <= height; ++h)
		{
			int gid = GetGid(w, h);
			auto it = grids_.find(gid);
			//没有初始化过就初始化一次
			if (it == grids_.end()) 
			{
				//std::vector<uint64_t> grid_entities = {};
				grids_.insert({ gid, {} });
				//printf("insert %d\n", gid);
			}
			
		}
	}
    */
   int g_x = floor(width_ * 1.0 / gx_);  //分成的格子数
   int g_y = floor(height_ * 1.0 / gy_);
   for (int i = 0; i < g_x; i++){
       for (int j = 0; j < g_y; j++){
           int gid = GxGy2Gid(i, j);
           std::shared_ptr<AOIGrid> ptr = std::make_shared<AOIGrid>(gid);
           //map_.insert<int, std::shared_ptr<AOIGrid>>(gid, ptr);
           map_[gid] = ptr;
       }
   }

}

void AOIManager::EnterBroadcast(const Entity& entity, int x, int y){
    std::unordered_set<int> enter_grids;
    ViewGrids(enter_grids, x, y);
    enter_message(entity, enter_grids);
}

void AOIManager::LeaveBroadcast(const Entity& entity, int x, int y){
    std::unordered_set<int> leave_grids;
    ViewGrids(leave_grids, x, y);
    leave_message(entity, leave_grids);
}

void AOIManager::MoveBroadcast(const Entity& entity, int x, int y){
    std::unordered_set<int> move_grids;
    ViewGrids(move_grids, x, y);
    move_message(entity, move_grids);
}

int AOIManager::GetGid(int x, int y) const{
    int g_x = x / gx_;  //所属的格子id
    int g_y = y / gy_;
    return GxGy2Gid(g_x, g_y);
}

int AOIManager::GxGy2Gid(int g_x, int g_y) const{
    return g_x * FACTOR + g_y;
}

/*
* x,y视野内的格子，就是九宫格
* */
void AOIManager::ViewGrids(std::unordered_set<int>& view_grids, int x, int y){
    int gx = x / gx_;
    int gy = y / gy_;
    view_grids.insert({
        GxGy2Gid(gx,gy)
        ,GxGy2Gid(gx,gy-1)
        ,GxGy2Gid(gx,gy+1)
        ,GxGy2Gid(gx-1,gy-1)
        ,GxGy2Gid(gx-1,gy)
        ,GxGy2Gid(gx-1,gy+1)
        ,GxGy2Gid(gx+1,gy-1)
        ,GxGy2Gid(gx+1,gy)
        ,GxGy2Gid(gx+1,gy+1)
    });
}

/*
* gid格子内的Entity
* */
void AOIManager::GridEntities(std::unordered_set<uint64_t>& entities, int gid){
    /*
    auto grid_it = grids_.find(gid);
    if (grid_it != grids_.end()){
        for (auto grid_entity_id : grid_it->second){
            entities.insert(grid_entity_id);
        }
    }
    */
   auto grid_it = map_.find(gid);
   if (grid_it != map_.end()){
       grid_it->second->GridEntities(entities);
   }
}

void AOIManager::move_cross_grid(const Entity& entity, int x, int y){
    std::unordered_set<int> leave_grids, enter_grids, move_grids;
    
    int gx1 = entity.x() / gx_;
	int gy1 = entity.y() / gy_;
	int gx2 = x / gx_;
	int gy2 = y / gy_;
    if(gx1 == gx2 && gy1 == gy2)return;

    std::unordered_set<int> old_view_grids,new_view_grids;
    ViewGrids(old_view_grids, entity.x(), entity.y());
    ViewGrids(new_view_grids, x, y);
    for (auto old_grid_id : old_view_grids){
        auto it = new_view_grids.find(old_grid_id);
        if (it == new_view_grids.end()){
            leave_grids.insert(old_grid_id);
        }
        else{
            move_grids.insert(old_grid_id);
        }
    }

    for (auto new_grid_id : new_view_grids){
        auto it = old_view_grids.find(new_grid_id);
        if (it == old_view_grids.end()){
            enter_grids.insert(new_grid_id);
        }
    }

    leave_message(entity, leave_grids);
    enter_message(entity, enter_grids);
    move_message(entity, move_grids);
}

void AOIManager::leave_message(const Entity& entity, std::unordered_set<int> leave_grids){
    std::unordered_set<uint64_t> leave_entities;
    for (auto it : leave_grids){
        GridEntities(leave_entities, it);
    }

    for (auto id : leave_entities){
        if (leaveMessageCB_)
            leaveMessageCB_(entity, id);
        //printf("leave message->to entity:%lu, entityid=%lu,x=%d,y=%d\n",id,entity.id(),entity.x(), entity.y());
    }
}

void AOIManager::enter_message(const Entity& entity, std::unordered_set<int> enter_grids){
    std::unordered_set<uint64_t> enter_entities;
    for (auto it : enter_grids){
        GridEntities(enter_entities, it);
    }

    for (auto id : enter_entities){
        if (enterMessageCB_)
            enterMessageCB_(entity, id);
        //printf("enter message->to entity:%lu, entityid=%lu,x=%d,y=%d\n",id,entity.id(),entity.x(), entity.y());
    }
}

void AOIManager::move_message(const Entity& entity, std::unordered_set<int> move_grids){
    std::unordered_set<uint64_t> move_entities;
    for (auto it : move_grids){
        GridEntities(move_entities, it);
    }

    for (auto id : move_entities){
        if (moveMessageCB_)
            moveMessageCB_(entity, id);
        //printf("move message->to entity:%lu, entityid=%lu,x=%d,y=%d\n",id,entity.id(),entity.x(), entity.y());
    }
}

}
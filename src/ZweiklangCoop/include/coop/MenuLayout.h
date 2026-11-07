#pragma once
#include <vector>
#include <algorithm>
namespace zweiklang {
struct MenuRow { int y, height; };
// Find a free line without moving any existing row, including disabled entries.
inline int FindMenuSlot(std::vector<MenuRow> rows,int height,int bottom=7800) {
    if(rows.empty()||height<=0)return -1;
    std::sort(rows.begin(),rows.end(),[](const MenuRow& a,const MenuRow& b){return a.y<b.y;});
    int spacing=(std::max)(height+40,300);
    for(size_t i=1;i<rows.size();++i) {
        const int gap=rows[i].y-rows[i-1].y;
        if(gap>=height+40)spacing=(std::max)(height+40,(std::min)(spacing,gap));
    }
    const int after=(std::max)(rows.back().y+spacing,rows.back().y+rows.back().height+40);
    if(after+height<=bottom)return after;
    for(size_t i=1;i<rows.size();++i) {
        const int candidate=rows[i-1].y+rows[i-1].height+40;
        if(candidate+height+40<=rows[i].y)return candidate;
    }
    const int before=rows.front().y-height-40;
    return before>=0?before:-1;
}
}

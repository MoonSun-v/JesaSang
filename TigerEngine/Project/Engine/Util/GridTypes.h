#pragma once

struct GridCoord
{
    int cx = 0;
    int cy = 0;

    bool operator==(const GridCoord& other) const
    {
        return cx == other.cx && cy == other.cy;
    }

    bool operator!=(const GridCoord& other) const
    {
        return !(*this == other);
    }
};
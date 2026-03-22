// qmlcache recognizes library pragma with a leading dot
.pragma library
// JS 自动规划库：在四边形区域生成均匀分布的内点并连成航线

// 简单的航线自动规划器：使用四边形区域生成均匀分布的内点，并串联起点到终点

function lerp(p1, p2, t) {
    return {
        lon: p1.lon + (p2.lon - p1.lon) * t,
        lat: p1.lat + (p2.lat - p1.lat) * t
    }
}

function buildGrid(corners, count) {
    if (count <= 0) return [];
    const cols = Math.max(1, Math.ceil(Math.sqrt(count)));
    const rows = Math.max(1, Math.ceil((count + cols - 1) / cols));
    let grid = [];
    for (let r = 0; r < rows; ++r) {
        const tRow = rows === 1 ? 0.5 : (r / (rows - 1));
        const left = lerp(corners[0], corners[3], tRow);
        const right = lerp(corners[1], corners[2], tRow);
        let row = [];
        for (let c = 0; c < cols; ++c) {
            const tCol = cols === 1 ? 0.5 : (c / (cols - 1));
            row.push(lerp(left, right, tCol));
        }
        if (r % 2 === 1) row.reverse();
        grid = grid.concat(row);
    }
    return grid.slice(0, count);
}

function computeRoute(corners, waypointCount) {
    if (!corners || corners.length < 4)
        return [];
    const start = corners[0];
    const end = corners[corners.length - 1];
    const n = Math.max(0, waypointCount);

    const eps = 1e-9;
    const innerPoints = buildGrid(corners, n).filter(function(p){
        const ds = Math.abs(p.lon - start.lon) + Math.abs(p.lat - start.lat);
        const de = Math.abs(p.lon - end.lon) + Math.abs(p.lat - end.lat);
        return ds > eps && de > eps;
    });
    if (!innerPoints.length && n > 0)
        return [];

    return [start].concat(innerPoints).concat([end]);
}

// 导出
var RouteAutoPlanner = {
    computeRoute: computeRoute
};

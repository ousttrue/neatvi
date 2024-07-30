pub fn strchr(str: []u8, ch: u8) ?[]u8 {
    for (str, 0..) |*p, i| {
        if (p.* == ch) {
            return str[i..];
        }
    }
    return null;
}

pub fn strrchr(str: []u8, ch: u8) ?[]u8 {
    var i: i32 = @intCast(str.len - 1);
    while (i >= 0) : (i -= 1) {
        if (str[@intCast(i)] == ch) {
            return str[@intCast(i)..];
        }
    }
    return null;
}


module servo() {
	union() {
		translate([5,40+4,6]) cylinder(10,4/2,4/2);
		translate([15,40+4,6]) cylinder(10,4/2,4/2);
		translate([5,-4,6]) cylinder(10,4/2,4/2);
		translate([15,-4,6]) cylinder(10,4/2,4/2);
		translate([0,-1/2,0])cube([20,41,20]);
		
	}
}

difference() {

// doska s bocnym uchytom	
union() {
	cube([120 , 140, 3]);
	translate([120-3,0,0]) {
		cube([3,140, 30]);
	}
	translate([120-7,0,3]) rotate(45, [0,1,0]){
		cube([3,140, 6]);
	}
}


translate([5,   5,-2.5]) cylinder(10, 4/2, 4/2);
translate([5+67,5,-2.5]) cylinder(10, 4/2, 4/2);
translate([5,   5+105,-2.5]) cylinder(10, 4/2, 4/2);
translate([5+67,5+105,-2.5]) cylinder(10, 4/2, 4/2);
/*
translate([120-6,5+13-10,10]) hull() {
	rotate(90, [0, 1,0]) cylinder(15, 4/2, 4/2);
	translate([0,0,15]) rotate(90, [0, 1,0]) cylinder(15, 4/2, 4/2);
}
translate([120-6,5+13+10,10]) hull() {
	rotate(90, [0, 1,0]) cylinder(15, 4/2, 4/2);
	translate([0,0,15]) rotate(90, [0, 1,0]) cylinder(15, 4/2, 4/2);
}

translate([120-6,5+13-10+110-36,10]) hull() {
	rotate(90, [0, 1,0]) cylinder(15, 4/2, 4/2);
	translate([0,0,15]) rotate(90, [0, 1,0]) cylinder(15, 4/2, 4/2);
}
translate([120-6,5+13+10+110-36,10]) hull() {
	rotate(90, [0, 1,0]) cylinder(15, 4/2, 4/2);
	translate([0,0,15]) rotate(90, [0, 1,0]) cylinder(15, 4/2, 4/2);
}
*/

translate([120-6,5,12]) hull() {
	rotate(90, [0, 1,0]) cylinder(15, 4/2, 4/2);
	translate([0,130,0]) rotate(90, [0, 1, 0]) cylinder(15, 4/2, 4/2);
}
translate([120-6,5,22]) hull() {
	rotate(90, [0, 1,0]) cylinder(15, 4/2, 4/2);
	translate([0,130,0]) rotate(90, [0, 1, 0]) cylinder(15, 4/2, 4/2);
}

translate([67/2-5,105+5+18-10,-10]) servo();
translate([67/2-5,5+20-10,-10]) servo();
}

module servo() {
	union() {
		translate([5,40+4,6]) cylinder(10,4/2,4/2);
		translate([15,40+4,6]) cylinder(10,4/2,4/2);
		translate([5,-4,6]) cylinder(10,4/2,4/2);
		translate([15,-4,6]) cylinder(10,4/2,4/2);
		translate([-1,-1,0])cube([22,42,20]);
		
	}
}


difference() {
	cube([80,60,2]);
	
	translate([50,10,-10]) servo();
	
	translate([5,10,-1]) hull() {
		cylinder(15, 4/2, 4/2);
		translate([30,0,0]) cylinder(15, 4/2, 4/2);
	}
	translate([5,50,-1]) hull() {
		cylinder(15, 4/2, 4/2);
		translate([30,0,0]) cylinder(15, 4/2, 4/2);
	}
}
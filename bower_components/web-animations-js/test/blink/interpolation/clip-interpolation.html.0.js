
assertInterpolation({
  property: 'clip',
  from: 'rect(0px, 75px, 80px, 10px)',
  to: 'rect(0px, 100px, 90px, 5px)'
}, [
  {at: -1, is: 'rect(0px, 50px, 70px, 15px)'},
  {at: 0, is: 'rect(0px, 75px, 80px, 10px)'},
  {at: 0.25, is: 'rect(0px, 81.25px, 82.5px, 8.75px)'},
  {at: 0.75, is: 'rect(0px, 93.75px, 87.5px, 6.25px)'},
  {at: 1, is: 'rect(0px, 100px, 90px, 5px)'},
  {at: 2, is: 'rect(0px, 125px, 100px, 0px)'},
]);

assertInterpolation({
  property: 'clip',
  from: 'rect(auto, auto, auto, 10px)',
  to: 'rect(20px, 50px, 50px, auto)'
}, [
  {at: -1, is: 'rect(auto, auto, auto, 10px)'},
  {at: 0, is: 'rect(auto, auto, auto, 10px)'},
  {at: 0.25, is: 'rect(auto, auto, auto, 10px)'},
  {at: 0.75, is: 'rect(20px, 50px, 50px, auto)'},
  {at: 1, is: 'rect(20px, 50px, 50px, auto)'},
  {at: 2, is: 'rect(20px, 50px, 50px, auto)'}
]);

assertInterpolation({
  property: 'clip',
  from: 'rect(auto, 0px, auto, 10px)',
  to: 'rect(auto, 50px, 50px, auto)'
}, [
  {at: -1, is: 'rect(auto, -50px, auto, 10px)'},
  {at: 0, is: 'rect(auto, 0px, auto, 10px)'},
  {at: 0.25, is: 'rect(auto, 12.5px, auto, 10px)'},
  {at: 0.75, is: 'rect(auto, 37.5px, 50px, auto)'},
  {at: 1, is: 'rect(auto, 50px, 50px, auto)'},
  {at: 2, is: 'rect(auto, 100px, 50px, auto)'}
]);

assertInterpolation({
  property: 'clip',
  from: 'auto',
  to: 'rect(0px, 50px, 50px, 0px)'
}, [
  {at: -1, is: 'auto'},
  {at: 0, is: 'auto'},
  {at: 0.25, is: 'auto'},
  {at: 0.75, is: 'rect(0px, 50px, 50px, 0px)'},
  {at: 1, is: 'rect(0px, 50px, 50px, 0px)'},
  {at: 2, is: 'rect(0px, 50px, 50px, 0px)'}
]);

assertInterpolation({
  property: 'clip',
  from: 'rect(0px, 50px, 50px, 0px)',
  to: 'auto'
}, [
  {at: -1, is: 'rect(0px, 50px, 50px, 0px)'},
  {at: 0, is: 'rect(0px, 50px, 50px, 0px)'},
  {at: 0.25, is: 'rect(0px, 50px, 50px, 0px)'},
  {at: 0.75, is: 'auto'},
  {at: 1, is: 'auto'},
  {at: 2, is: 'auto'},
]);

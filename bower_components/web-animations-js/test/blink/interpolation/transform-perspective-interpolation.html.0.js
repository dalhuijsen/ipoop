

assertInterpolation({
  property: 'transform',
  prefixedProperty: ['-webkit-transform'],
  from: 'perspective(400px)',
  to: 'perspective(500px)'
}, [
  {at: -1, is: 'perspective(300px)'},
  {at: 0, is: 'perspective(400px)'},
  {at: 0.25, is: 'perspective(425px)'},
  {at: 0.75, is: 'perspective(475px)'},
  {at: 1, is: 'perspective(500px)'},
  {at: 2, is: 'perspective(600px)'},
]);
assertInterpolation({
  property: 'transform',
  prefixedProperty: ['-webkit-transform'],
  from: 'skewX(10rad) perspective(400px)',
  to: 'skewX(20rad) perspective(500px)'
}, [
  {at: -1, is: 'skewX(0rad) perspective(300px)'},
  {at: 0, is: 'skewX(10rad) perspective(400px)'},
  {at: 0.25, is: 'skewX(12.5rad) perspective(425px)'},
  {at: 0.75, is: 'skewX(17.5rad) perspective(475px)'},
  {at: 1, is: 'skewX(20rad) perspective(500px)'},
  {at: 2, is: 'skewX(30rad) perspective(600px)'},
]);
assertInterpolation({
  property: 'transform',
  prefixedProperty: ['-webkit-transform'],
  from: 'scaleZ(1) perspective(400px)',
  to: 'scaleZ(2) perspective(500px)'
}, [
  {at: -1, is: 'scaleZ(0) perspective(300px)'},
  {at: 0, is: 'scaleZ(1) perspective(400px)'},
  {at: 0.25, is: 'scaleZ(1.25) perspective(425px)'},
  {at: 0.75, is: 'scaleZ(1.75) perspective(475px)'},
  {at: 1, is: 'scaleZ(2) perspective(500px)'},
  {at: 2, is: 'scaleZ(3) perspective(600px)'},
]);



assertInterpolation({
  property: 'transform',
  prefixedProperty: ['-webkit-transform'],
  from: 'skewX(10rad)',
  to: 'skewX(20rad)'
}, [
  {at: -1, is: 'skewX(0rad)'},
  {at: 0, is: 'skewX(10rad)'},
  {at: 0.25, is: 'skewX(12.5rad)'},
  {at: 0.75, is: 'skewX(17.5rad)'},
  {at: 1, is: 'skewX(20rad)'},
  {at: 2, is: 'skewX(30rad)'},
]);
assertInterpolation({
  property: 'transform',
  prefixedProperty: ['-webkit-transform'],
  from: 'skewY(10rad)',
  to: 'skewY(20rad)'
}, [
  {at: -1, is: 'skewY(0rad)'},
  {at: 0, is: 'skewY(10rad)'},
  {at: 0.25, is: 'skewY(12.5rad)'},
  {at: 0.75, is: 'skewY(17.5rad)'},
  {at: 1, is: 'skewY(20rad)'},
  {at: 2, is: 'skewY(30rad)'},
]);

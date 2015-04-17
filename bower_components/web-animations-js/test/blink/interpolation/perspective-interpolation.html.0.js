
assertInterpolation({
  property: 'perspective',
  prefixedProperty: ['-webkit-perspective'],
  from: '50px',
  to: '100px'
}, [
  {at: -20, is: 'none'}, // perspective does not accept 0 or negative values
  {at: -1, is: 'none'}, // perspective does not accept 0 or negative values
  {at: -0.3, is: '35px'},
  {at: 0, is: '50px'},
  {at: 0.3, is: '65px'},
  {at: 0.6, is: '80px'},
  {at: 1, is: '100px'},
  {at: 1.5, is: '125px'}
]);

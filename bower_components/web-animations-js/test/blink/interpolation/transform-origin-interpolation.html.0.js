
assertInterpolation({
  property: 'transform-origin',
  prefixedProperty: ['-webkit-transform-origin'],
  from: '0% 50% 5px',
  to: '100% 150% 0px'
}, [
  {at: -0.3, is: '-30% 20% 6.5px'},
  {at: 0, is: '0% 50% 5px'},
  {at: 0.3, is: '30% 80% 3.5px'},
  {at: 0.6, is: '60% 110% 2px'},
  {at: 1, is: '100% 150% 0px'},
  {at: 1.5, is: '150% 200% -2.5px'}
]);

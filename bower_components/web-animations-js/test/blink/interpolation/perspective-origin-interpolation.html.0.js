
assertInterpolation({
  property: 'perspective-origin',
  prefixedProperty: ['-webkit-perspective-origin'],
  from: '0% 50%',
  to: '100% 150%'
}, [
  {at: -0.3, is: '-30% 20%'},
  {at: 0, is: '0% 50%'},
  {at: 0.3, is: '30% 80%'},
  {at: 0.6, is: '60% 110%'},
  {at: 1, is: '100% 150%'},
  {at: 1.5, is: '150% 200%'}
]);
